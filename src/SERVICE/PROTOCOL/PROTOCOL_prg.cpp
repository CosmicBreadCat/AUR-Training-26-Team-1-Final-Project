#include "PROTOCOL_int.h"
#include <math.h>
#include <string.h>

static const char *const kModeNames[] = {"STOP", "WHEEL_VEL", "TWIST",   "MOVE",
                                         "TURN", "SQUARE",    "APPROACH"};

static const char *const kResultNames[] = {"RUNNING", "DONE", "FAILED",
                                           "PREEMPTED"};

const char *PROTOCOL_pcModeName(RobotMode mode) {
  return kModeNames[(uint8_t)mode];
}

const char *PROTOCOL_pcResultName(Result result) {
  return kResultNames[(uint8_t)result];
}

static bool parseModeName(const char *name, RobotMode &out) {
  for (uint8_t i = 0; i < sizeof(kModeNames) / sizeof(kModeNames[0]); i++) {
    if (strcmp(name, kModeNames[i]) == 0) {
      out = (RobotMode)i;
      return true;
    }
  }
  return false;
}

static bool isNumber(JsonVariantConst v) {
  return v.is<float>(); // true for integers too
}

bool Protocol::decode(const uint8_t *buf, size_t len, Command &out) {
  if (deserializeJson(doc_, (const char *)buf, len)) {
    error_ = "bad_json";
    return false;
  }
  JsonObjectConst obj = doc_.as<JsonObjectConst>();
  if (obj.isNull()) {
    error_ = "not_object";
    return false;
  }
  if (!obj["type"].is<const char *>() || !obj["seq"].is<uint32_t>() ||
      !obj["t"].is<uint32_t>()) {
    error_ = "bad_header";
    return false;
  }

  out = Command{};
  out.seq = obj["seq"];
  out.t = obj["t"];
  const char *type = obj["type"];

  if (strcmp(type, "twist") == 0) {
    if (!isNumber(obj["v"]) || !isNumber(obj["omega"])) {
      error_ = "bad_twist";
      return false;
    }
    out.type = MsgType::Twist;
    out.v = obj["v"];
    out.omega = obj["omega"];
  } else if (strcmp(type, "wheel_vel") == 0) {
    if (!isNumber(obj["wl"]) || !isNumber(obj["wr"])) {
      error_ = "bad_wheel_vel";
      return false;
    }
    out.type = MsgType::WheelVel;
    out.wl = obj["wl"];
    out.wr = obj["wr"];
  } else if (strcmp(type, "mode") == 0) {
    if (!obj["mode"].is<const char *>() ||
        !parseModeName(obj["mode"], out.mode)) {
      error_ = "bad_mode";
      return false;
    }
    out.type = MsgType::Mode;
    out.params.distance = obj["distance"] | NAN;
    out.params.vMax = obj["v_max"] | NAN;
    out.params.aMax = obj["a_max"] | NAN;
    out.params.angle = obj["angle"] | NAN;
    out.params.omegaMax = obj["omega_max"] | NAN;
    out.params.alphaMax = obj["alpha_max"] | NAN;
    out.params.maxSearch = obj["max_search"] | NAN;
    out.params.vSearch = obj["v_search"] | NAN;
    out.params.grabDistance = obj["grab_distance"] | NAN;
  } else if (strcmp(type, "heartbeat") == 0) {
    out.type = MsgType::Heartbeat;
  } else if (strcmp(type, "ping") == 0) {
    out.type = MsgType::Ping;
  } else {
    error_ = "unknown_type";
    return false;
  }

  error_ = "";
  return true;
}

void Protocol::begin(const char *type, uint32_t seq, uint32_t t) {
  doc_.clear();
  doc_["type"] = type;
  doc_["seq"] = seq;
  doc_["t"] = t;
}

size_t Protocol::finish(uint8_t *buf, size_t cap) {
  if (doc_.overflowed() || measureJson(doc_) >= cap) {
    return 0;
  }
  return serializeJson(doc_, (char *)buf, cap);
}

size_t Protocol::encodeOdometry(const Odometry &odom, uint32_t seq, uint32_t t,
                                uint8_t *buf, size_t cap) {
  begin("odometry", seq, t);
  doc_["x"] = odom.x;
  doc_["y"] = odom.y;
  doc_["theta"] = odom.theta;
  doc_["v"] = odom.v;
  doc_["omega"] = odom.omega;
  return finish(buf, cap);
}

size_t Protocol::encodeStatus(const Status &status, uint32_t seq, uint32_t t,
                              uint8_t *buf, size_t cap) {
  begin("status", seq, t);
  doc_["mode"] = PROTOCOL_pcModeName(status.mode);
  doc_["phase"] = status.phase;
  doc_["result"] = PROTOCOL_pcResultName(status.result);
  doc_["reason"] = status.reason;
  doc_["cmd_seq"] = status.cmdSeq;
  doc_["watchdog"] = status.watchdog;
  doc_["rx_ok"] = status.rxOk;
  doc_["rx_bad"] = status.rxBad;
  return finish(buf, cap);
}

size_t Protocol::encodeAck(uint32_t ackSeq, bool ok, const char *err,
                           uint32_t seq, uint32_t t, uint8_t *buf, size_t cap) {
  begin("ack", seq, t);
  doc_["ack_seq"] = ackSeq;
  doc_["ok"] = ok;
  doc_["err"] = err;
  return finish(buf, cap);
}

size_t Protocol::encodePong(uint32_t pingT, uint32_t seq, uint32_t t,
                            uint8_t *buf, size_t cap) {
  begin("pong", seq, t);
  doc_["ping_t"] = pingT;
  return finish(buf, cap);
}
