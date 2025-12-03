#include "NTPCom.hpp"

NTPCom::NTPCom(const String &server,
               long gmtOffsetSeconds,
               unsigned long updateIntervalMs)
    : client(nullptr),
      ntpServer(server),
      gmtOffsetSec(gmtOffsetSeconds),
      updateInterval(updateIntervalMs),
      baseEpochMs(0ULL),
      synced(false)
{
}

NTPCom::~NTPCom() {
    if (client != nullptr) {
        delete client;
        client = nullptr;
    }
}

void NTPCom::begin() {
    if (client == nullptr) {
        // Creamos el NTPClient usando el WiFiUDP interno
        client = new NTPClient(udp,
                               ntpServer.c_str(),
                               gmtOffsetSec,
                               updateInterval);
        client->begin();
    }
}

bool NTPCom::sync() {
    if (client == nullptr) {
        return false;
    }

    if (client->update()) {
        // epoch en segundos -> ms
        unsigned long long epochMs =
            (unsigned long long)client->getEpochTime() * 1000ULL;

        // Guardamos base tal que getTimeMs ≈ epochMs
        baseEpochMs = epochMs - (unsigned long long)millis();
        synced = true;

        Serial.printf("[NTPCom] NTP sincronizado: %llu ms\n", epochMs);
        return true;
    }

    Serial.println("[NTPCom] Falló la actualización NTP");
    return false;
}

unsigned long long NTPCom::getTimeMs() const {
    if (!synced) {
        // Si aún no hay NTP, devolvemos algo consistente (millis)
        return (unsigned long long)millis();
    }

    return baseEpochMs + (unsigned long long)millis();
}

bool NTPCom::isSynced() const {
    return synced;
}
