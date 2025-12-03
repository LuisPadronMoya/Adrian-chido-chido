#include <Arduino.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <WiFi.h>

// Módulos
#include "NTPCom.hpp"
#include "FirebaseCom.hpp"
#include "MeshCom.hpp"
#include "Protocol.hpp"

// ---------- CONFIG (AJUSTAR) ----------
#define MESH_PREFIX "videoetica"
#define MESH_PASSWORD "12345678"
#define MESH_PORT 5555

// WiFi (root - ajustar)
const char* WIFI_SSID = "LabPercepcion_Guest";
const char* WIFI_PASSWORD = "";

// Firebase (ajustar)
#define DATABASE_URL "partixmillon-default-rtdb.firebaseio.com" // sin https
#define API_KEY ""

// ---------- OBJETOS ----------
NTPCom* ntpCom = nullptr;
FirebaseCom* firebaseCom = nullptr;
MeshCom* meshCom = nullptr;

// FreeRTOS queue para pasar mensajes mesh -> firebase
QueueHandle_t firebaseQueue;

struct MeshMessage {
  uint32_t fromId;
  char jsonString[512]; // aumentar según mensajes
};

// ---------- FUNCIONES DE TIEMPO ----------
unsigned long long getRootTime() {
  // Si ya tenemos NTP, usamos ese tiempo. Si no, millis.
  if (ntpCom != nullptr) {
    return ntpCom->getTimeMs();
  }
  return (unsigned long long)millis();
}

void syncInternetTime() {
  if (ntpCom != nullptr) {
    ntpCom->sync();
  }
}

// ---------- MESH: RECEPCION ----------
void receivedCallback(uint32_t from, String &msg) {
  // Parseamos JSON
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial.println("[ROOT] JSON inválido recibido, ignorando");
    return;
  }

  // ---- 1) Petición NTP interna desde child (NTP_REQ) ----
  if (Protocol::isNtpReq(doc)) {
    // T2: instante en que ROOT recibió la petición
    unsigned long long T2 = getRootTime();

    // T1: eco del child
    unsigned long long T1 = (unsigned long long)(doc["t1"] | 0ULL);

    // T3: instante en que ROOT envía la respuesta
    unsigned long long T3 = getRootTime();

    // Construir respuesta con el módulo de protocolo
    DynamicJsonDocument res(256);
    Protocol::makeNtpRes(res, T1, T2, T3);

    String out;
    serializeJson(res, out);

    // Responder solo al child que hizo la petición
    if (meshCom != nullptr) {
      meshCom->sendSingle(from, out);
    }
    Serial.printf("[ROOT] NTP_REQ de %u -> respondido (T2=%llu, T3=%llu)\n", from, T2, T3);
    return;
  }

  // ---- 2) Datos de sensores (DATA / DATA_HIST) ----
  if (Protocol::isData(doc)) {
    // Empaquetar en MeshMessage y encolar a firebaseQueue
    MeshMessage item;
    item.fromId = from;
    memset(item.jsonString, 0, sizeof(item.jsonString));
    // Copiamos el mensaje (cuidar tamaño)
    msg.toCharArray(item.jsonString, sizeof(item.jsonString));

    BaseType_t ok = xQueueSend(firebaseQueue, &item, 0); // no-blocking
    if (ok == pdTRUE) {
      Serial.printf("[ROOT] Mensaje DATA encolado desde %u\n", from);
    } else {
      Serial.println("[ROOT] firebaseQueue llena, mensaje descartado o persistir"); 
      // Recomendación: persistir en disco si es crítico
    }
    return;
  }

  // ---- 3) Otros tipos -> procesar según necesidad ----
  String type = Protocol::getType(doc);
  Serial.print("[ROOT] Mensaje con tipo no manejado: ");
  Serial.println(type);
}

// ---------- TAREA: MESH + LÓGICA PRINCIPAL (FreeRTOS) ----------
void meshLoopTask(void *parameter) {
  bool sentInitialSync = false;

  for (;;) {    
    if (meshCom != nullptr) {
      meshCom->update();
    }

    // SINCRONIZACIÓN NTP EXTERNA periódica (por si cambia)
    static unsigned long lastNTP = 0;
    if (millis() - lastNTP > 60000) { // cada 60s (ajustable)
      syncInternetTime();
      lastNTP = millis();
    }

    // Damos un pequeño delay para RTOS
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ---------- TAREA: FIREBASE (sube datos recibidos) ----------
void firebaseLoop(void *parameter) {
  MeshMessage item;
  for (;;) {
    if (xQueueReceive(firebaseQueue, &item, portMAX_DELAY) == pdTRUE) {
      // Construir path en RTDB: /nodos/<fromId>/lecturas
      String path = "/nodos/";
      path += String(item.fromId);
      path += "/lecturas";

      // Construir FirebaseJson a partir del string
      FirebaseJson json;
      json.setJsonData(String(item.jsonString));

      // Usamos el módulo del profe
      if (firebaseCom != nullptr) {
        firebaseCom->pushData(json, path);
      } else {
        Serial.println("[ROOT] firebaseCom == nullptr, no se puede subir a Firebase");
      }

      // Enviar ACK al child usando el módulo de protocolo
      DynamicJsonDocument ack(256);
      Protocol::makeAck(ack, getRootTime(), "OK");

      String ackStr;
      serializeJson(ack, ackStr);

      if (meshCom != nullptr) {
        meshCom->sendSingle(item.fromId, ackStr);
      }
    }
    // pequeño respiro
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ---------- HELPER: detectar canal del router ----------
int detectRouterChannel() {
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    if (WiFi.SSID(i) == WIFI_SSID) {
      return WiFi.channel(i);
    }
  }
  return 1; // default
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  // 1) Conectar WiFi (para NTP y Firebase)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("[ROOT] Conectando a WiFi...");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }


  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[ROOT] WiFi conectado.");

    // Iniciar módulo NTP
    String ntpServer = "pool.ntp.org";
    ntpCom = new NTPCom(ntpServer, 0, 60000);
    ntpCom->begin();
    ntpCom->sync();
  } else {
    Serial.println("[ROOT] No se pudo conectar a WiFi. Seguirá intentando en background.");
    // De todas formas la malla puede funcionar sin Internet para el NTP interno:
  }

  // 2) Iniciar Firebase usando la clase del profe
  String dbURL  = DATABASE_URL;
  String apiKey = API_KEY;
  firebaseCom   = new FirebaseCom(dbURL, apiKey);
  

  // 3) Inicializar mesh: detectar canal del router y usarlo mediante el módulo
  int channel = detectRouterChannel();
  Serial.printf("[ROOT] Inicializando mesh en canal %d\n", channel);

  String meshPrefixStr = MESH_PREFIX;
  String meshPassStr   = MESH_PASSWORD;
  String wifiSsidStr   = WIFI_SSID;
  String wifiPassStr   = WIFI_PASSWORD;

  meshCom = new MeshCom(meshPrefixStr, meshPassStr, MESH_PORT, wifiSsidStr, wifiPassStr);
  meshCom->initRoot(channel, &receivedCallback);
  meshCom->setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

  // 4) Crear cola y tareas FreeRTOS
  firebaseQueue = xQueueCreate(40, sizeof(MeshMessage));
  xTaskCreate(
    firebaseLoop,
    "FirebaseTask",
    9600,
    NULL,
    1,
    NULL
  );
  xTaskCreate(
    meshLoopTask,
    "MeshTask",
    9600,
    NULL,
    1,
    NULL
  );
  Serial.println("[ROOT] ROOT listo.");
}

// ---------- LOOP (no usado) ----------
void loop() {
}
