#ifndef COMM_INT_H_
#define COMM_INT_H_

#include "../PROTOCOL/PROTOCOL_int.h"
#include "../UDP/UDP_int.h"
#include "COMM_cfg.h"
#include <stdint.h>

/*
 * Robot side of the laptop link: UDP transport + JSON protocol + watchdog.
 * Answers ping and heartbeat itself and hands mode/twist/wheel_vel/gripper
 * commands to the caller. Not thread-safe: one task owns it.
 */
class Comms {
public:
  /** @brief Joins the router and opens the socket. @return false on failure. */
  bool begin();

  /**
   * @brief Non-blocking. Processes waiting packets until one needs the
   *        caller's attention. Call repeatedly until it returns false.
   * @param out Set to the next mode/twist/wheel_vel/gripper command.
   * @return true if out holds a command.
   */
  bool poll(Command &out);

  /** @brief false once no valid packet has arrived for COMM_WATCHDOG_MS. */
  bool linkAlive() const;

  void sendAck(uint32_t ackSeq, bool ok, const char *err);
  void sendOdometry(const Odometry &odom);
  void sendSensors(const Sensors &sensors);

  /** @brief Sends status. rxOk/rxBad are filled in here. */
  void sendStatus(Status status);

  uint32_t rxOk() const { return rxOk_; }
  uint32_t rxBad() const { return rxBad_; }

  /** @brief Why the most recent packet was rejected. */
  const char *lastReject() const { return lastReject_; }

  bool wifiConnected() const { return link_.wifiConnected(); }
  IPAddress localIp() const { return link_.localIp(); }

private:
  void sendBuffer(size_t len);

  UdpLink link_;
  Protocol proto_;
  uint8_t rxBuf_[UDP_MAX_PAYLOAD];
  uint8_t txBuf_[PROTOCOL_TX_BUF_SIZE];
  uint32_t lastSeq_ = 0;
  bool haveSeq_ = false;
  uint32_t txSeq_ = 0;
  uint32_t rxOk_ = 0;
  uint32_t rxBad_ = 0;
  const char *lastReject_ = "";
};

#endif /* COMM_INT_H_ */
