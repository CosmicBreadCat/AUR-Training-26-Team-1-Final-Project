#include <Arduino.h>
#include <WiFi.h>
#include "UDP_int.h"

bool UdpLink::begin() {
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);  // modem sleep adds tens of ms of latency
    if (!WiFi.softAP(UDP_AP_SSID, UDP_AP_PASS, UDP_AP_CHANNEL, 0, UDP_AP_MAX_CLIENTS)) {
        return false;
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

int UdpLink::stationCount() const {
    return WiFi.softAPgetStationNum();
}
