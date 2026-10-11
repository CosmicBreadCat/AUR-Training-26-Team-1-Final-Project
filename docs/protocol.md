# Robot ↔ Laptop Protocol (v1)

How the laptop talks to the robot. If you use `tools/robot_link.py`, it handles most of this for you (see [Using RobotLink](#using-robotlink)). Read this to understand what the robot does with your commands and what it reports back.

## Connection

| | |
|---|---|
| WiFi network | team router, SSID and password in `src/SERVICE/UDP/UDP_cfg.h` (2.4 GHz); the robot, laptop and camera phone all join it |
| Robot address | `192.168.69.50` (static, set in `src/SERVICE/UDP/UDP_cfg.h`), UDP port `5005` |
| Format | one JSON object per UDP packet, max 1472 bytes |
| Units | SI everywhere: m, rad, s, m/s, rad/s |

The robot replies to whichever address sent it the last valid packet, so **the laptop always talks first**.

UDP can lose packets. Anything that must arrive (mode changes) is acknowledged and retried. Everything else is streamed, so a lost packet is replaced by the next one.

## Common fields

Every message, in both directions, has:

| Field | Type | Meaning |
|---|---|---|
| `type` | string | message type (tables below) |
| `seq` | integer ≥ 0 | sender's message counter; must increase |
| `t` | integer ≥ 0 | sender's clock in ms, for timestamping and RTT |

## Laptop → robot

| `type` | Extra fields | When to send |
|---|---|---|
| `mode` | `mode`, plus that mode's parameters ([Modes](#modes)) | on change; the robot replies `ack` |
| `twist` | `v` (m/s), `omega` (rad/s, + = counter-clockwise) | 50 Hz while in TWIST |
| `wheel_vel` | `wl`, `wr` (rad/s) | 50 Hz while in WHEEL_VEL *(not implemented yet)* |
| `heartbeat` | — | 50 Hz whenever no stream above is being sent |
| `ping` | — | any time; the robot replies `pong` |

Examples:
```json
{"type":"mode","seq":101,"t":5230,"mode":"MOVE","distance":0.5,"v_max":0.3}
{"type":"twist","seq":102,"t":5250,"v":0.2,"omega":0.0}
{"type":"heartbeat","seq":103,"t":5270}
```

## Robot → laptop

| `type` | Fields | Rate |
|---|---|---|
| `odometry` | `x`, `y` (m), `theta` (rad), `v` (m/s), `omega` (rad/s) | 20 Hz |
| `status` | `mode`, `phase`, `result`, `reason`, `cmd_seq`, `watchdog`, `rx_ok`, `rx_bad` | 5 Hz, plus immediately on any change |
| `ack` | `ack_seq`, `ok`, `err` | reply to each `mode` |
| `pong` | `ping_t` (the `t` of the ping) | reply to each `ping` |

`odometry` is all zeros until the encoders are integrated.

`status` fields:

| Field | Meaning |
|---|---|
| `mode` | current mode |
| `phase` | what the mode is doing right now, e.g. `IDLE`, `DRIVING`, `NO_INPUT`, `SEARCH` |
| `result` | `RUNNING`, `DONE`, `FAILED` or `PREEMPTED` |
| `reason` | why it failed, e.g. `watchdog`, `timeout`, `stall`; empty otherwise |
| `cmd_seq` | `seq` of the `mode` command this result belongs to, so you can tell which command finished |
| `watchdog` | `true` while no valid packet has arrived for 300 ms |
| `rx_ok`, `rx_bad` | packets accepted / rejected since boot |

`ack` fields: `ok` is `false` when the robot refused the command, and `err` says why (`not_implemented` for modes not built yet).

Examples:
```json
{"type":"status","seq":88,"t":91200,"mode":"TWIST","phase":"DRIVING","result":"RUNNING","reason":"","cmd_seq":101,"watchdog":false,"rx_ok":412,"rx_bad":0}
{"type":"ack","seq":89,"t":91210,"ack_seq":101,"ok":true,"err":""}
```

## Rules

- **Validation.** The robot rejects a packet that isn't a JSON object, has an unknown `type`, is missing a required field, or has a wrong field type. A rejected packet counts in `rx_bad` and is otherwise ignored, with no reply.
- **Sequence numbers.** The robot drops a packet whose `seq` is not higher than the last one it accepted. Once the watchdog has tripped, any `seq` is accepted, so a restarted script can reconnect. RobotLink starts `seq` from the wall clock, so restarts are accepted right away.
- **Watchdog.** Any valid packet keeps the link alive. After **300 ms** with no valid packet, the robot switches to STOP and reports `result: FAILED, reason: watchdog`. It stays in STOP until it receives a new `mode` command.
- **Stream staleness.** In TWIST, if no `twist` arrives for 300 ms, the robot sets v and ω to 0 (`phase: NO_INPUT`), even if heartbeats keep the link alive.
- **Preemption.** A new `mode` command replaces the current one immediately. The old command reports `PREEMPTED` with its own `cmd_seq`.
- **Repeats.** Sending the mode the robot is already running (e.g. a retry after a lost ack) is acknowledged and only updates `cmd_seq`.
- **On entering any mode,** the robot resets its controllers and takes its current position and heading as the starting point.

## Modes

Only **STOP** and **TWIST** are implemented right now. The other modes are acknowledged with `ok: false, err: "not_implemented"` until they're built. Parameters marked `?` are optional.

### STOP
The safe resting state. The robot boots into STOP, and returns to it after DONE, FAILED or a watchdog trip.
- **Parameters:** none.
- **Behavior:** ramps the speed down to zero, then releases the motors.
- **Ends:** only when a new `mode` command arrives.

### TWIST
Continuous driving from the laptop, e.g. a joystick or your own path controller.
- **Parameters:** none. Then stream `twist` at 50 Hz.
- **Behavior:**
  - follows `v` and `omega`, with acceleration limits;
  - when `omega = 0`, holds the current heading using the gyro, so "straight" stays straight.
- **Phases:** `NO_INPUT` (no fresh twist; stopped), `DRIVING`.
- **Ends:** never on its own.

### WHEEL_VEL *(not implemented)*
Testing only: commands each wheel directly. Used to characterize the motors and tune the wheel controllers.
- **Parameters:** none. Then stream `wheel_vel` at 50 Hz.
- **Ends:** never on its own. If the stream stops, both wheels go to 0.

### MOVE *(not implemented)*
Drive a set distance in a straight line.
- **Parameters:** `distance` (m, negative = backwards), `v_max?` (m/s), `a_max?` (m/s²).
- **Behavior:** a smooth speed profile over the distance (measured by the wheel encoders), while holding the starting heading (gyro).
- **DONE:** within 1 cm of the target and stopped.
- **FAILED:**
  - `timeout`: took more than twice the planned time;
  - `stall`: motors at full power but not moving;
  - `obstacle`: the ultrasonic sensor saw something too close ahead.

### TURN *(not implemented)*
Rotate in place.
- **Parameters:** `angle` (rad, relative, + = counter-clockwise), `omega_max?` (rad/s), `alpha_max?` (rad/s²).
- **Behavior:** a smooth turn-rate profile (measured by the gyro), while preventing forward creep.
- **DONE:** within 1° of the target and stopped.
- **FAILED:** `timeout`, `stall`.

### SQUARE *(not implemented)*
Drive slowly forward to a line on the floor and align square to it, using the two IR line sensors on the front corners. This also re-references the robot's heading.
- **Parameters:** `max_search` (m), `v_search?` (m/s).
- **Phases:**
  1. `SEARCH`: driving forward.
  2. `FIRST_TRIP`: one sensor has hit the line.
  3. `SECOND_TRIP`: both sensors have hit; the angle is computed.
  4. `ALIGN`: turning square.
- **DONE:** aligned.
- **FAILED:**
  - `no_line`: no line within `max_search`;
  - `one_side_only`: the second sensor never hit, meaning the line is too steep or has a gap;
  - `timeout`.

### APPROACH *(not implemented)*
Close in on a box to grabbing distance, using the ultrasonic sensor.
- **Parameters:** `grab_distance` (m), `v_max?` (m/s).
- **Behavior:** slows smoothly to stop at `grab_distance`, holding heading. Optionally the laptop can steer it with a camera bearing; that message will be added when this mode is built.
- **DONE:** within 1 cm of `grab_distance` and stopped.
- **FAILED:**
  - `lost_target`: no ultrasonic echo for several readings;
  - `max_travel`: drove too far without arriving;
  - `timeout`.

## Using RobotLink

`tools/robot_link.py` needs Python 3.10+. It:
- sends heartbeats or your twist at 50 Hz in the background;
- fills in `seq` and `t`;
- retries mode commands until they're acknowledged;
- keeps the latest odometry and status for you.

```python
import time
from robot_link import RobotLink

with RobotLink() as robot:
    ok, err = robot.set_mode("TWIST")      # (False, "timeout") if the robot never answered
    robot.set_twist(0.2, 0.0)              # streamed until you change it
    time.sleep(2)
    print(robot.latest_odometry())         # dict, or None if none received yet
    print(robot.latest_status())
    print(robot.ping(), "ms")

    ok, err = robot.set_mode("MOVE", distance=0.5)   # currently (False, "not_implemented")
    robot.set_mode("STOP")
```

To check on a command, compare `latest_status()["cmd_seq"]` with the command you sent, and look at its `result`. When your script exits, the background stream stops, and the robot's watchdog brings it to STOP within 300 ms.
