#include "COMM_int.h"
#include <Arduino.h>

bool Comms::begin() { return link_.begin(); }

bool Comms::poll(Command &out) {
  size_t n;
  while ((n = link_.receive(rxBuf_, sizeof(rxBuf_))) > 0) {
    Command cmd;
    if (!proto_.decode(rxBuf_, n, cmd)) {
      rxBad_++;
      lastReject_ = proto_.error();
      continue;
    }
    // Drop old or duplicate packets. Once the link is dead any seq is
    // accepted, so a restarted laptop script can reconnect.
    if (haveSeq_ && cmd.seq <= lastSeq_ && linkAlive()) {
      rxBad_++;
      lastReject_ = "old_seq";
      continue;
    }

    lastSeq_ = cmd.seq;
    haveSeq_ = true;
    link_.acceptSender();
    rxOk_++;

    switch (cmd.type) {
    case MsgType::Heartbeat:
      break; // accepting it already fed the watchdog
    case MsgType::Ping:
      sendBuffer(
          proto_.encodePong(cmd.t, ++txSeq_, millis(), txBuf_, sizeof(txBuf_)));
      break;
    default:
      out = cmd;
      return true;
    }
  }
  return false;
}

bool Comms::linkAlive() const {
  return link_.msSinceValidRx() < COMM_WATCHDOG_MS;
}

void Comms::sendAck(uint32_t ackSeq, bool ok, const char *err) {
  sendBuffer(proto_.encodeAck(ackSeq, ok, err, ++txSeq_, millis(), txBuf_,
                              sizeof(txBuf_)));
}

void Comms::sendOdometry(const Odometry &odom) {
  sendBuffer(
      proto_.encodeOdometry(odom, ++txSeq_, millis(), txBuf_, sizeof(txBuf_)));
}

void Comms::sendSensors(const Sensors &sensors) {
  sendBuffer(
      proto_.encodeSensors(sensors, ++txSeq_, millis(), txBuf_, sizeof(txBuf_)));
}

void Comms::sendStatus(Status status) {
  status.rxOk = rxOk_;
  status.rxBad = rxBad_;
  sendBuffer(
      proto_.encodeStatus(status, ++txSeq_, millis(), txBuf_, sizeof(txBuf_)));
}

void Comms::sendBuffer(size_t len) {
  if (len > 0) {
    link_.send(txBuf_, len); // no-op until a laptop has connected
  }
}
