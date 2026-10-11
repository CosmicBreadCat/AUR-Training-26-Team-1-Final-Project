#include <Arduino.h>
#include <WiFi.h>
#include "UDP_int.h"

bool UdpLink::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);  // modem sleep adds tens of ms of latency
    WiFi.setAutoReconnect(true);
#if UDP_USE_STATIC_IP
    if (!WiFi.config(IPAddress(UDP_STATIC_IP), IPAddress(UDP_GATEWAY), IPAddress(UDP_SUBNET))) {
        return false;
    }
#endif
    WiFi.begin(UDP_WIFI_SSID, UDP_WIFI_PASS);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < UDP_CONNECT_TIMEOUT_MS) {
        delay(100);
    }
    return udp_.begin(UDP_LOCAL_PORT) == 1;
}

size_t UdpLink::receive(uint8_t *buf, size_t cap) {
    int len = udp_.parsePacket();
    if (len <= 0 || (size_t)len > cap) {
        return 0;  // nothing waiting, or oversized (dropped by the next parsePacket)
    }
    int n = udp_.read(buf, cap);
    senderIp_ = udp_.remoteIP();
    senderPort_ = udp_.remotePort();
    return n > 0 ? (size_t)n : 0;
}

void UdpLink::acceptSender() {
    peerIp_ = senderIp_;
    peerPort_ = senderPort_;
    lastValidRxMs_ = millis();
}

bool UdpLink::send(const uint8_t *data, size_t len) {
    if (!hasPeer()) {
        return false;
    }
    if (!udp_.beginPacket(peerIp_, peerPort_)) {
        return false;
    }
    udp_.write(data, len);
    return udp_.endPacket() == 1;
}

uint32_t UdpLink::msSinceValidRx() const {
    return hasPeer() ? millis() - lastValidRxMs_ : UINT32_MAX;
}

bool UdpLink::wifiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

IPAddress UdpLink::localIp() const {
    return WiFi.localIP();
}
