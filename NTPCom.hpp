#pragma once

#include <WiFiUdp.h>
#include <NTPClient.h>

class NTPCom {
public:
    // server: ej. "pool.ntp.org"
    // gmtOffsetSeconds: offset horario (ej. 0)
    // updateIntervalMs: cada cuánto intenta actualizar (ms)
    NTPCom(const String &server,
           long gmtOffsetSeconds,
           unsigned long updateIntervalMs);

    ~NTPCom();

    // Inicia el cliente NTP (debe llamarse cuando ya hay WiFi)
    void begin();

    // Fuerza una sincronización con el servidor NTP
    // Regresa true si se logró actualizar la hora
    bool sync();

    // Tiempo actual en milisegundos desde época (como tu getRootTime)
    unsigned long long getTimeMs() const;

    // Indica si ya se sincronizó al menos una vez
    bool isSynced() const;

private:
    WiFiUDP udp;
    NTPClient *client;      // se crea en begin()

    String ntpServer;
    long gmtOffsetSec;
    unsigned long updateInterval;

    unsigned long long baseEpochMs;  // equivalente a tu serverEpochMs
    bool synced;
};
