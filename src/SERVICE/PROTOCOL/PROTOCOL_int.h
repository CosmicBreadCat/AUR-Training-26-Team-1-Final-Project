#ifndef PROTOCOL_INT_H_
#define PROTOCOL_INT_H_

#include <stdint.h>
#include <stddef.h>
#include <ArduinoJson.h>
#include "PROTOCOL_cfg.h"

/* Laptop -> robot message types */
enum class MsgType : uint8_t {
    Mode,
    Twist,
    WheelVel,
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
enum class Result : uint8_t {
    Running,
    Done,
    Failed,
    Preempted
};

/* Optional `mode` parameters. Absent ones are NAN. */
struct ModeParams {
    float distance;      // MOVE (m)
    float vMax;          // MOVE, APPROACH (m/s)
    float aMax;          // MOVE (m/s^2)
    float angle;         // TURN, relative (rad)
    float omegaMax;      // TURN (rad/s)
    float alphaMax;      // TURN (rad/s^2)
    float maxSearch;     // SQUARE (m)
    float vSearch;       // SQUARE (m/s)
    float grabDistance;  // APPROACH (m)
};

/* A decoded laptop -> robot message. Only the fields for `type` are set. */
struct Command {
    MsgType   type;
    uint32_t  seq;
    uint32_t  t;
    RobotMode mode;      // Mode
    ModeParams params;   // Mode
    float     v;         // Twist (m/s)
    float     omega;     // Twist (rad/s)
    float     wl;        // WheelVel (rad/s)
    float     wr;        // WheelVel (rad/s)
};

struct Odometry {
    float x;      // m
    float y;      // m
    float theta;  // rad
    float v;      // m/s
    float omega;  // rad/s
};

struct Status {
    RobotMode   mode;
    const char *phase;   // mode-specific, e.g. "IDLE", "DRIVING"
    Result      result;
    const char *reason;  // "" unless Failed
    uint32_t    cmdSeq;  // seq of the mode command this result belongs to
    bool        watchdog;
    uint32_t    rxOk;
    uint32_t    rxBad;
};

const char *PROTOCOL_pcModeName(RobotMode mode);
const char *PROTOCOL_pcResultName(Result result);

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
    size_t encodeOdometry(const Odometry &odom, uint32_t seq, uint32_t t, uint8_t *buf, size_t cap);
    size_t encodeStatus(const Status &status, uint32_t seq, uint32_t t, uint8_t *buf, size_t cap);
    size_t encodeAck(uint32_t ackSeq, bool ok, const char *err, uint32_t seq, uint32_t t, uint8_t *buf, size_t cap);
    size_t encodePong(uint32_t pingT, uint32_t seq, uint32_t t, uint8_t *buf, size_t cap);

private:
    void begin(const char *type, uint32_t seq, uint32_t t);
    size_t finish(uint8_t *buf, size_t cap);

    JsonDocument doc_;
    const char *error_ = "";
};

#endif /* PROTOCOL_INT_H_ */
