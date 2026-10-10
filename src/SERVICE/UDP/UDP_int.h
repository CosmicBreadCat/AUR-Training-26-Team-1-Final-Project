#ifndef UDP_INT_H_
#define UDP_INT_H_

#include <stdint.h>
#include <stddef.h>
#include <WiFiUdp.h>
#include "UDP_cfg.h"

/*
 * Moves bytes between the ESP32 and the laptop over the SoftAP.
 * Knows nothing about packet contents: the protocol layer validates packets
 * and calls acceptSender() for the good ones.
 * Not thread-safe: only one task should own an instance.
 */
class UdpLink {
public:
    /**
     * @brief Starts the SoftAP and binds the socket to UDP_LOCAL_PORT.
     * @return false if either step fails.
     */
    bool begin();

    /**
     * @brief Non-blocking. Copies the next waiting datagram into buf and
     *        remembers its sender (untrusted until acceptSender()).
     * @param buf Destination buffer.
     * @param cap Size of buf in bytes.
     * @return Payload length, or 0 if nothing is waiting or it didn't fit.
     */
    size_t receive(uint8_t *buf, size_t cap);

    /**
     * @brief Promotes the sender of the last received packet to peer and
     *        resets the watchdog timer. Call only after the packet is validated.
     */
    void acceptSender();

    /**
     * @brief Sends a datagram to the current peer.
     * @return false if there is no peer yet or the send failed.
     */
    bool send(const uint8_t *data, size_t len);

    /** @brief true once at least one packet has been accepted. */
    bool hasPeer() const { return peerPort_ != 0; }

    /** @brief Address and port replies go to (valid once hasPeer()). */
    IPAddress peerIp() const { return peerIp_; }
    uint16_t peerPort() const { return peerPort_; }

    /**
     * @brief Time since the last accepted packet, for the comms watchdog.
     * @return Milliseconds, or UINT32_MAX if no packet has been accepted yet.
     */
    uint32_t msSinceValidRx() const;

    /** @brief Number of stations connected to the SoftAP. */
    int stationCount() const;

private:
    WiFiUDP   udp_;
    IPAddress senderIp_;        // source of the last received packet (untrusted)
    uint16_t  senderPort_ = 0;
    IPAddress peerIp_;          // source of the last accepted packet
    uint16_t  peerPort_ = 0;
    uint32_t  lastValidRxMs_ = 0;
};

#endif /* UDP_INT_H_ */
