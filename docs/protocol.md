# Robot ↔ Laptop Protocol (v1)

How the laptop talks to the robot. If you use `tools/robot_link.py`, it handles most of this for you (see [Using RobotLink](#using-robotlink)). Read this to understand what the robot does with your commands and what it reports back.

## Connection

| | |
|---|---|
| WiFi network | team router, SSID and password in `src/SERVICE/UDP/UDP_cfg.h` (2.4 GHz); the robot, laptop and camera phone all join it |
| Robot address | `192.168.69.50` (static, set in `src/SERVICE/UDP/UDP_cfg.h`), UDP port `5005` |
| Format | one JSON object per UDP packet, max 1472 bytes |
| Units | SI everywhere: m, rad, s, m/s, rad/s |
| Arena frame | see [Arena frame](#arena-frame): origin bottom-left corner, x → EAST, y → NORTH |

The robot replies to whichever address sent it the last valid packet, so **the laptop always talks first**.

UDP can lose packets. Anything that must arrive (mode, gripper and square_ref commands) is acknowledged and retried. Everything else is streamed, so a lost packet is replaced by the next one.

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
| `gripper` | `lift` (`"UP"`/`"DOWN"`), `grip` (`"OPEN"`/`"CLOSED"`); each optional, at least one | on change; the robot replies `ack` ([Gripper](#gripper)) |
| `square_ref` | `side` (`"NORTH"`/`"EAST"`/`"SOUTH"`/`"WEST"`): the edge SQUARE just aligned to | when SQUARE is in phase `WAITING`; the robot replies `ack` ([SQUARE](#square)) |
| `heartbeat` | — | 50 Hz whenever no stream above is being sent |
| `ping` | — | any time; the robot replies `pong` |

Examples:
```json
{"type":"mode","seq":101,"t":5230,"mode":"MOVE","distance":0.5,"v_max":0.3}
{"type":"twist","seq":102,"t":5250,"v":0.2,"omega":0.0}
{"type":"gripper","seq":104,"t":5290,"lift":"DOWN","grip":"CLOSED"}
{"type":"square_ref","seq":105,"t":5310,"side":"SOUTH"}
{"type":"heartbeat","seq":103,"t":5270}
```

## Robot → laptop

| `type` | Fields | Rate |
|---|---|---|
| `odometry` | `x`, `y` (m), `theta` (rad), `v` (m/s), `omega` (rad/s) | 20 Hz |
| `sensors` | `range_m` (ultrasonic range in m, or `null` when there's no reading) | 10 Hz |
| `status` | `mode`, `phase`, `result`, `reason`, `cmd_seq`, `lift`, `grip`, `watchdog`, `rx_ok`, `rx_bad` | 5 Hz, plus immediately on any change |
| `ack` | `ack_seq`, `ok`, `err` | reply to each `mode` and `gripper` |
| `pong` | `ping_t` (the `t` of the ping) | reply to each `ping` |

`odometry` is all zeros until the encoders are integrated, and `sensors.range_m` is always `null` until the ultrasonic driver exists.

`status` fields:

| Field | Meaning |
|---|---|
| `mode` | current mode |
| `phase` | what the mode is doing right now, e.g. `IDLE`, `DRIVING`, `NO_INPUT`, `SEARCH` |
| `result` | `RUNNING`, `DONE`, `FAILED` or `PREEMPTED` |
| `reason` | why it failed, e.g. `watchdog`, `timeout`, `stall`; empty otherwise |
| `cmd_seq` | `seq` of the `mode` command this result belongs to, so you can tell which command finished |
| `lift`, `grip` | the gripper's current commanded state (`UP`/`DOWN`, `OPEN`/`CLOSED`) |
| `watchdog` | `true` while no valid packet has arrived for 300 ms |
| `rx_ok`, `rx_bad` | packets accepted / rejected since boot |

`ack` fields: `ok` is `false` when the robot refused the command, and `err` says why (`not_implemented` for modes not built yet).

Examples:
```json
{"type":"status","seq":88,"t":91200,"mode":"TWIST","phase":"DRIVING","result":"RUNNING","reason":"","cmd_seq":101,"lift":"UP","grip":"OPEN","watchdog":false,"rx_ok":412,"rx_bad":0}
{"type":"ack","seq":89,"t":91210,"ack_seq":101,"ok":true,"err":""}
```

## Rules

- **Validation.** The robot rejects a packet that isn't a JSON object, has an unknown `type`, is missing a required field, or has a wrong field type. A rejected packet counts in `rx_bad` and is otherwise ignored, with no reply.
- **Sequence numbers.** The robot drops a packet whose `seq` is not higher than the last one it accepted. Once the watchdog has tripped, any `seq` is accepted, so a restarted script can reconnect. RobotLink starts `seq` from the wall clock, so restarts are accepted right away.
- **Watchdog.** Any valid packet keeps the link alive. After **300 ms** with no valid packet, the robot switches to STOP and reports `result: FAILED, reason: watchdog`. It stays in STOP until it receives a new `mode` command. The gripper is **not** affected: it holds its state.
- **Stream staleness.** In TWIST, if no `twist` arrives for 300 ms, the robot sets v and ω to 0 (`phase: NO_INPUT`), even if heartbeats keep the link alive.
- **Preemption.** A new `mode` command replaces the current one immediately. The old command reports `PREEMPTED` with its own `cmd_seq`.
- **Repeats.** Sending the mode the robot is already running (e.g. a retry after a lost ack) is acknowledged and changes nothing: `cmd_seq` stays that of the command that started the mode, so the mode isn't restarted.
- **End of a mode.** When a mode reports DONE or FAILED, the robot returns to STOP, keeping that command's `cmd_seq`, `result` and `reason` in the status.
- **On entering any mode,** the robot resets its controllers and takes its current position and heading as the starting point.

## Arena frame

The robot keeps its own pose in the arena, and reports it in `odometry`:
- **Origin:** the bottom-left (south-west) corner of the square arena.
- **Axes:** `x` points EAST, `y` points NORTH, in meters.
- **Heading:** `theta` is measured from EAST, counter-clockwise positive. Facing EAST is 0, NORTH is +π/2, WEST is π, SOUTH is −π/2.

At boot the pose is a rough estimate (near the bottom-left corner, facing NORTH). It becomes exact by squaring on the arena edges: see [SQUARE](#square). Squaring is only done on the four boundary edges.

**Localizing at the start** (from the bottom-left corner):
1. Face the SOUTH edge → `set_mode("SQUARE")` → wait for phase `WAITING` → `set_square_side("SOUTH")`. θ and y are now exact.
2. `set_mode("TURN", facing="WEST")` → SQUARE → wait for `WAITING` → `set_square_side("WEST")`. x is now exact too.

Square on any edge again later to remove odometry drift.

## Modes

Only **STOP** and **TWIST** are fully implemented right now. **MOVE**, **TURN** and **SQUARE** are accepted (`ok: true`), but their driving code is a stub:
- **MOVE and TURN** immediately report `FAILED` and return to STOP. The reasons are listed under each mode.
- **SQUARE** skips the driving phases and goes straight to `WAITING`. From there on it works for real: `square_ref` corrects the pose and SQUARE ends with DONE.

**WHEEL_VEL** and **APPROACH** are acknowledged with `ok: false, err: "not_implemented"` until they're built. Parameters marked `?` are optional.

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

### MOVE *(accepted; stub reports `not_implemented`)*
Drive a set distance in a straight line.
- **Parameters:** `distance` (m, negative = backwards), `v_max?` (m/s), `a_max?` (m/s²).
- **Behavior:** a smooth speed profile over the distance (measured by the wheel encoders), while holding the starting heading (gyro).
- **DONE:** within 1 cm of the target and stopped.
- **FAILED:**
  - `timeout`: took more than twice the planned time;
  - `stall`: motors at full power but not moving;
  - `obstacle`: the ultrasonic sensor saw something too close ahead.

### TURN *(accepted; stub reports `not_implemented`)*
Rotate in place, either by an angle or to face an arena side.
- **Parameters:** exactly one of:
  - `angle`: rad, relative, + = counter-clockwise;
  - `facing`: `"NORTH"`/`"EAST"`/`"SOUTH"`/`"WEST"`, absolute in the [arena frame](#arena-frame). Meant for the D-pad.

  Optional: `omega_max?` (rad/s), `alpha_max?` (rad/s²).
- **Behavior:** a smooth turn-rate profile (measured by the gyro), while preventing forward creep.
- **DONE:** within 1° of the target and stopped.
- **FAILED:**
  - `missing_angle`: neither `angle` nor `facing` given;
  - `conflicting_target`: both given;
  - `no_heading_ref`: `facing` used before any `square_ref` since boot, so the robot doesn't know where NORTH is yet;
  - `timeout`, `stall`;
  - `not_implemented`: what the stub reports when the target is valid.

### SQUARE *(accepted; driving phases are a stub, goes straight to `WAITING`)*
Drive slowly forward to an arena edge line and align square to it, using the two IR line sensors on the front corners. Then the driver says which edge it was, and the robot corrects its pose.
- **Parameters:** `max_search` (m), `v_search?` (m/s).
- **Phases:**
  1. `SEARCH`: driving forward.
  2. `FIRST_TRIP`: one sensor has hit the line.
  3. `SECOND_TRIP`: both sensors have hit; the angle is computed.
  4. `ALIGN`: turning square.
  5. `WAITING`: stopped, waiting for `square_ref`. **There's no timeout.** To cancel, send any other mode (the robot reports PREEMPTED and the pose is unchanged).
- **`square_ref`:** only accepted in `WAITING`, otherwise it's acked `ok: false, err: "not_waiting"`. The robot is facing the named edge with its IR sensors on the line, so it sets (`S` = arena size, `d` = IR sensor offset ahead of the robot's center):

  | `side` | `theta` | Coordinate |
  |---|---|---|
  | NORTH | +π/2 | `y = S − d` |
  | SOUTH | −π/2 | `y = d` |
  | EAST | 0 | `x = S − d` |
  | WEST | π | `x = d` |

  The other coordinate is unchanged. Then SQUARE ends with DONE. If the ack of a `square_ref` is lost, the retry gets `not_waiting` (SQUARE has already finished), so check the status for `result: DONE`, not only the ack.
- **DONE:** aligned and the pose corrected.
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

## Gripper

Two servos, commanded independently of the drive mode with a `gripper` message:
- **`lift`:** `UP` or `DOWN` (the angle servo).
- **`grip`:** `OPEN` or `CLOSED` (the state servo).

Rules:
- **Each field is optional.** Leave one out to keep that servo where it is. A message with neither, or with an unknown state, is rejected (`bad_gripper`, no ack).
- **It works in any mode**, including while driving in TWIST, and never changes the drive mode.
- **It holds through mode changes and watchdog trips,** so a WiFi blip doesn't drop a box.
- **At boot** the robot assumes `UP` / `OPEN`.
- **Current status: mock.** The robot acks, logs and reports the state, but the servos aren't driven yet. The robot maps each state to a servo angle itself, so laptop code never deals with angles.

## Using RobotLink

`tools/robot_link.py` needs Python 3.10+. It:
- sends heartbeats or your twist at 50 Hz in the background;
- fills in `seq` and `t`;
- retries mode and gripper commands until they're acknowledged;
- keeps the latest odometry, sensors and status for you.

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

    ok, err = robot.set_gripper(lift="DOWN", grip="CLOSED")  # works in any mode
    print(robot.latest_sensors())          # {"range_m": None, ...} until the ultrasonic driver exists

    # Localize: square on the SOUTH edge
    robot.set_mode("SQUARE", max_search=0.5)
    while robot.latest_status()["phase"] != "WAITING":
        time.sleep(0.01)
    robot.set_square_side("SOUTH")         # theta and y corrected in the next odometry
    robot.set_mode("TURN", facing="WEST")  # D-pad: face an arena side

    ok, err = robot.set_mode("MOVE", distance=0.5)   # acked, then status: FAILED/not_implemented (stub)
    robot.set_mode("STOP")
```

To check on a command, compare `latest_status()["cmd_seq"]` with the command you sent, and look at its `result`. When your script exits, the background stream stops, and the robot's watchdog brings it to STOP within 300 ms.
