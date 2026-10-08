#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// SoftAP settings. The password must be at least 8 characters or softAP()
// fails.
static const char *AP_SSID = "AUR-Team1-Robot";
static const char *AP_PASS = "robot1234";
static const int AP_CHANNEL = 6;     // stick to 1, 6 or 11
static const int AP_MAX_CLIENTS = 1; // only the laptop

static const uint16_t UDP_PORT = 5005;

WiFiUDP udp;
uint8_t rxBuf[1472]; // largest UDP payload that fits in one 1500-byte frame
int lastStationCount = -1;

void setup() {
  Serial.begin(115200);
  delay(200);

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false); // modem sleep adds tens of ms of latency
  if (!WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, 0, AP_MAX_CLIENTS)) {
    Serial.println("softAP() failed");
    while (true)
      delay(1000);
  }
  Serial.printf("AP \"%s\" up, IP %s\n", AP_SSID,
                WiFi.softAPIP().toString().c_str());

  udp.begin(UDP_PORT); // bind: datagrams for port 5005 land in this socket
  Serial.printf("Listening for UDP on port %u\n", UDP_PORT);
}

void loop() {
  int stations = WiFi.softAPgetStationNum();
  if (stations != lastStationCount) {
    Serial.printf("Stations connected: %d\n", stations);
    lastStationCount = stations;
  }

  // parsePacket() pulls the next datagram out of the socket's receive buffer
  // and returns its size, or 0 if none is waiting.
  if (udp.parsePacket() <= 0)
    return;

  int n = udp.read(rxBuf, sizeof(rxBuf));
  IPAddress from = udp.remoteIP();
  uint16_t fromPort = udp.remotePort();

  // Echo first, log after, so Serial doesn't add to the round-trip time.
  udp.beginPacket(from, fromPort);
  udp.write(rxBuf, n);
  udp.endPacket();

  Serial.printf("%s:%u  %d bytes: %.*s\n", from.toString().c_str(), fromPort, n,
                n, (const char *)rxBuf);
}
