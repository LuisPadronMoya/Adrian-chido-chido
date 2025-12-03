#include "MeshCom.hpp"

MeshCom::MeshCom(const String &prefix,
                 const String &password,
                 uint16_t port,
                 const String &ssid,
                 const String &wifiPass)
    : meshPrefix(prefix),
      meshPassword(password),
      meshPort(port),
      wifiSsid(ssid),
      wifiPassword(wifiPass)
{
}

MeshCom::~MeshCom() {
    // nada especial que liberar por ahora
}

void MeshCom::initRoot(int channel, void (*receivedCallback)(uint32_t from, String &msg)) {
    // Inicializamos la malla en modo ROOT, como tenías en el código original
    mesh.init(meshPrefix.c_str(),
              meshPassword.c_str(),
              &scheduler,
              meshPort,
              WIFI_AP_STA,
              channel);

    mesh.stationManual(wifiSsid.c_str(), wifiPassword.c_str());
    mesh.setHostname("RootNode");
    mesh.onReceive(receivedCallback);
    mesh.setRoot(true);
    mesh.setContainsRoot(true);
}

void MeshCom::update() {
    mesh.update();
}

void MeshCom::sendSingle(uint32_t nodeId, const String &msg) {
    mesh.sendSingle(nodeId, msg);
}

void MeshCom::setDebugMsgTypes(uint8_t types) {
    mesh.setDebugMsgTypes(types);
}
