#ifndef PROTOCOL_INT_H_
#define PROTOCOL_INT_H_

#include "PROTOCOL_cfg.h"
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

/* Laptop -> robot message types */
enum class MsgType : uint8_t {
  Mode,
  Twist,
  WheelVel,
  Gripper,
  SquareRef,
  Heartbeat,
  Ping
};

/* Robot behaviors, selected by a `mode` message */
enum class RobotMode : uint8_t {
  Stop,
  WheelVel,
  Twist,
  Move,
  Turn,
  Square,
  Approach
};

/* Outcome of the command named by Status::cmdSeq */
enum class Result : uint8_t { Running, Done, Failed, Preempted };

/* Gripper servo states, set by a `gripper` message */
enum class LiftState : uint8_t { Up, Down };
enum class GripState : uint8_t { Open, Closed };

/* Arena boundary edges, used by `square_ref` and TURN `facing` */
enum class Side : uint8_t { North, East, South, West };

/* Optional `mode` parameters. Absent numbers are NAN. */
struct ModeParams {
  float distance;     // MOVE (m)
  float vMax;         // MOVE, APPROACH (m/s)
  float aMax;         // MOVE (m/s^2)
  float angle;        // TURN, relative (rad)
  float omegaMax;     // TURN (rad/s)
  float alphaMax;     // TURN (rad/s^2)
  float maxSearch;    // SQUARE (m)
  float vSearch;      // SQUARE (m/s)
  float grabDistance; // APPROACH (m)
  bool hasFacing;     // TURN: facing was given
  Side facing;        // TURN, absolute: face this arena side
};

/* A decoded laptop -> robot message. Only the fields for `type` are set. */
struct Command {
  MsgType type;
  uint32_t seq;
  uint32_t t;
  RobotMode mode;    // Mode
  ModeParams params; // Mode
  float v;           // Twist (m/s)
  float omega;       // Twist (rad/s)
  float wl;          // WheelVel (rad/s)
  float wr;          // WheelVel (rad/s)
  bool hasLift;      // Gripper: lift was given
  LiftState lift;    // Gripper
  bool hasGrip;      // Gripper: grip was given
  GripState grip;    // Gripper
  Side side;         // SquareRef: the edge the robot just squared on
};

struct Odometry {
  float x;     // m
  float y;     // m
  float theta; // rad
  float v;     // m/s
  float omega; // rad/s
};

struct Sensors {
  float rangeM; // ultrasonic range (m), NAN when there is no reading
  bool hasIr;   // false until the IR line sensors are read
  bool irLeft;  // front-left IR sees the line
  bool irRight; // front-right IR sees the line
};

struct Status {
  RobotMode mode;
  const char *phase; // mode-specific, e.g. "IDLE", "DRIVING"
  Result result;
  const char *reason; // "" unless Failed
  uint32_t cmdSeq;    // seq of the mode command this result belongs to
  LiftState lift;     // current commanded gripper state
  GripState grip;
  bool watchdog;
  uint32_t rxOk;
  uint32_t rxBad;
};

const char *PROTOCOL_pcModeName(RobotMode mode);
const char *PROTOCOL_pcResultName(Result result);
const char *PROTOCOL_pcLiftName(LiftState lift);
const char *PROTOCOL_pcGripName(GripState grip);
const char *PROTOCOL_pcSideName(Side side);

/*
 * JSON encode/decode for schema v1. No WiFi or clock dependency: callers
 * pass seq and t for outgoing messages.
 * Not thread-safe: it reuses one JsonDocument.
 */
class Protocol {
public:
  /**
   * @brief Parses and validates one laptop -> robot message.
   * @return false if the JSON, type or required keys are invalid;
   *         error() then says why.
   */
  bool decode(const uint8_t *buf, size_t len, Command &out);

  /** @brief Why the last decode() failed. */
  const char *error() const { return error_; }

  /**
   * @brief Encoders write one message into buf.
   * @return Bytes written, or 0 if it didn't fit.
   */
  size_t encodeOdometry(const Odometry &odom, uint32_t seq, uint32_t t,
                        uint8_t *buf, size_t cap);
  size_t encodeStatus(const Status &status, uint32_t seq, uint32_t t,
                      uint8_t *buf, size_t cap);
  size_t encodeSensors(const Sensors &sensors, uint32_t seq, uint32_t t,
                       uint8_t *buf, size_t cap);
  size_t encodeAck(uint32_t ackSeq, bool ok, const char *err, uint32_t seq,
                   uint32_t t, uint8_t *buf, size_t cap);
  size_t encodePong(uint32_t pingT, uint32_t seq, uint32_t t, uint8_t *buf,
                    size_t cap);

private:
  void begin(const char *type, uint32_t seq, uint32_t t);
  size_t finish(uint8_t *buf, size_t cap);

  JsonDocument doc_;
  const char *error_ = "";
};

#endif /* PROTOCOL_INT_H_ */
