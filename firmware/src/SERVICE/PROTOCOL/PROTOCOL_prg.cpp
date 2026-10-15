#include "PROTOCOL_int.h"
#include <math.h>
#include <string.h>

static const char *const kModeNames[] = {"STOP", "WHEEL_VEL", "TWIST",   "MOVE",
                                         "TURN", "SQUARE",    "APPROACH"};

static const char *const kResultNames[] = {"RUNNING", "DONE", "FAILED",
                                           "PREEMPTED"};

/* Each table must list names in the same order as its enum */
static const char *const kLiftNames[] = {"UP", "DOWN"};
static const char *const kGripNames[] = {"OPEN", "CLOSED"};
static const char *const kSideNames[] = {"NORTH", "EAST", "SOUTH", "WEST"};

#define COUNT_OF(arr) (sizeof(arr) / sizeof((arr)[0]))

const char *PROTOCOL_pcModeName(RobotMode mode) {
  return kModeNames[(uint8_t)mode];
}

const char *PROTOCOL_pcResultName(Result result) {
  return kResultNames[(uint8_t)result];
}

const char *PROTOCOL_pcLiftName(LiftState lift) {
  return kLiftNames[(uint8_t)lift];
}

const char *PROTOCOL_pcGripName(GripState grip) {
  return kGripNames[(uint8_t)grip];
}

const char *PROTOCOL_pcSideName(Side side) {
  return kSideNames[(uint8_t)side];
}

/* Finds name in a table; out is its index, i.e. the enum value */
static bool parseName(JsonVariantConst value, const char *const *names,
                      uint8_t count, uint8_t &out) {
  if (!value.is<const char *>()) {
    return false;
  }
  const char *name = value;
  for (uint8_t i = 0; i < count; i++) {
    if (strcmp(name, names[i]) == 0) {
      out = i;
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
    uint8_t mode;
    if (!parseName(obj["mode"], kModeNames, COUNT_OF(kModeNames), mode)) {
      error_ = "bad_mode";
      return false;
    }
    out.type = MsgType::Mode;
    out.mode = (RobotMode)mode;
    out.params.distance = obj["distance"] | NAN;
    out.params.vMax = obj["v_max"] | NAN;
    out.params.aMax = obj["a_max"] | NAN;
    out.params.angle = obj["angle"] | NAN;
    out.params.omegaMax = obj["omega_max"] | NAN;
    out.params.alphaMax = obj["alpha_max"] | NAN;
    out.params.maxSearch = obj["max_search"] | NAN;
    out.params.vSearch = obj["v_search"] | NAN;
    out.params.grabDistance = obj["grab_distance"] | NAN;
    out.params.hasFacing = !obj["facing"].isNull();
    if (out.params.hasFacing) {
      uint8_t facing;
      if (!parseName(obj["facing"], kSideNames, COUNT_OF(kSideNames), facing)) {
        error_ = "bad_mode";
        return false;
      }
      out.params.facing = (Side)facing;
    }
  } else if (strcmp(type, "gripper") == 0) {
    // Each field is optional (missing = leave that servo as is), but a present
    // one must be a known state, and at least one must be present.
    uint8_t lift = 0, grip = 0;
    out.hasLift = !obj["lift"].isNull();
    out.hasGrip = !obj["grip"].isNull();
    if ((!out.hasLift && !out.hasGrip) ||
        (out.hasLift &&
         !parseName(obj["lift"], kLiftNames, COUNT_OF(kLiftNames), lift)) ||
        (out.hasGrip &&
         !parseName(obj["grip"], kGripNames, COUNT_OF(kGripNames), grip))) {
      error_ = "bad_gripper";
      return false;
    }
    out.type = MsgType::Gripper;
    out.lift = (LiftState)lift;
    out.grip = (GripState)grip;
  } else if (strcmp(type, "square_ref") == 0) {
    uint8_t side;
    if (!parseName(obj["side"], kSideNames, COUNT_OF(kSideNames), side)) {
      error_ = "bad_square_ref";
      return false;
    }
    out.type = MsgType::SquareRef;
    out.side = (Side)side;
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
  doc_["lift"] = PROTOCOL_pcLiftName(status.lift);
  doc_["grip"] = PROTOCOL_pcGripName(status.grip);
  doc_["watchdog"] = status.watchdog;
  doc_["rx_ok"] = status.rxOk;
  doc_["rx_bad"] = status.rxBad;
  return finish(buf, cap);
}

size_t Protocol::encodeSensors(const Sensors &sensors, uint32_t seq,
                               uint32_t t, uint8_t *buf, size_t cap) {
  begin("sensors", seq, t);
  if (isnan(sensors.rangeM)) {
    doc_["range_m"] = nullptr; // no reading
  } else {
    doc_["range_m"] = sensors.rangeM;
  }
  if (sensors.hasIr) {
    doc_["ir_left"] = sensors.irLeft;
    doc_["ir_right"] = sensors.irRight;
  } else {
    doc_["ir_left"] = nullptr; // not read yet
    doc_["ir_right"] = nullptr;
  }
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
