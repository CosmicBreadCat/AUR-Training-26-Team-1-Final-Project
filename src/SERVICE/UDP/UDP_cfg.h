#ifndef UDP_CFG_H_
#define UDP_CFG_H_

/*
 * SoftAP settings
 */
#define UDP_AP_SSID           "AUR-Team1-Robot"
#define UDP_AP_PASS           "robot1234"   // must be >= 8 chars or softAP() fails
#define UDP_AP_CHANNEL        6             // stick to 1, 6 or 11
#define UDP_AP_MAX_CLIENTS    1             // only the laptop

/*
 * Socket settings
 */
#define UDP_LOCAL_PORT        5005
#define UDP_MAX_PAYLOAD       1472          // largest payload that fits in one 1500-byte frame

#endif /* UDP_CFG_H_ */
