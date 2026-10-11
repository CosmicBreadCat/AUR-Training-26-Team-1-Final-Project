#ifndef UDP_CFG_H_
#define UDP_CFG_H_

/*
 * Router (station mode) settings. The ESP32 is 2.4 GHz only, so this must be
 * the router's 2.4 GHz network.
 */
#define UDP_WIFI_SSID "Ahashek_El"
#define UDP_WIFI_PASS "Gambarii"

/*
 * Static address so the laptop always knows where the robot is. Must be in the
 * router's subnet and outside its DHCP pool (or reserved for the robot's MAC).
 * Set UDP_USE_STATIC_IP to 0 to take whatever DHCP hands out instead; the IP
 * is printed on the serial monitor either way.
 */
#define UDP_USE_STATIC_IP 1
#define UDP_STATIC_IP 192, 168, 69, 50
#define UDP_GATEWAY 192, 168, 69, 1
#define UDP_SUBNET 255, 255, 255, 0

/* How long begin() waits for the first connection before carrying on.
   The link keeps retrying in the background after that. */
#define UDP_CONNECT_TIMEOUT_MS 10000

/*
 * Socket settings
 */
#define UDP_LOCAL_PORT 5005
#define UDP_MAX_PAYLOAD 1472 // largest payload that fits in one 1500-byte frame

#endif /* UDP_CFG_H_ */
