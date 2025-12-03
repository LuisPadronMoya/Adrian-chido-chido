#pragma once

#include "painlessMesh.h"
#include <WiFi.h>

class MeshCom {
public:
    MeshCom(const String &prefix,
            const String &password,
            uint16_t port,
            const String &wifiSsid,
            const String &wifiPassword);

    ~MeshCom();

    // Inicializa el nodo ROOT de la malla
    void initRoot(int channel, void (*receivedCallback)(uint32_t from, String &msg));

    // Llamar periódicamente en la tarea de mesh
    void update();

    // Enviar mensaje a un nodo
    void sendSingle(uint32_t nodeId, const String &msg);

    // Set de niveles de debug (ERROR | STARTUP | CONNECTION, etc.)
    void setDebugMsgTypes(uint8_t types);

private:
    Scheduler scheduler;
    painlessMesh mesh;

    String meshPrefix;
    String meshPassword;
    uint16_t meshPort;
    String wifiSsid;
    String wifiPassword;
};
