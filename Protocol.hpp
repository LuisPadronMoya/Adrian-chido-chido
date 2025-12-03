#pragma once

#include <ArduinoJson.h>

namespace Protocol {

    // Tipos de mensaje
    extern const char* const TYPE_NTP_REQ;
    extern const char* const TYPE_NTP_RES;
    extern const char* const TYPE_ACK;
    extern const char* const TYPE_DATA_PREFIX;

    // Obtiene el campo "type" como String (si no existe, regresa "")
    String getType(const JsonDocument& doc);

    // ¿Es un mensaje NTP_REQ?
    bool isNtpReq(const JsonDocument& doc);

    // ¿Es un mensaje de datos (empieza con "DATA")?
    bool isData(const JsonDocument& doc);

    // Construye un mensaje NTP_RES dentro de un JsonDocument ya creado
    void makeNtpRes(DynamicJsonDocument& doc,
                    unsigned long long t1,
                    unsigned long long t2,
                    unsigned long long t3);

    // Construye un ACK genérico: { "type": "ACK", "status": status, "server_ts": ts }
    void makeAck(DynamicJsonDocument& doc,
                 unsigned long long serverTs,
                 const char* status = "OK");
}
