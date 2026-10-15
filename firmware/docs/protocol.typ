// RobotLink protocol: laptop <-> robot message format.
// Compile with: typst compile docs/protocol.typ

#set document(title: "RobotLink Protocol", author: "AUR Training 26, Team 1")
#set page(
  paper: "a4",
  margin: (x: 2cm, y: 2.2cm),
  numbering: "1",
  header: context {
    if counter(page).get().first() > 1 [
      #set text(size: 8.5pt, fill: luma(110))
      RobotLink Protocol v1 #h(1fr) AUR Training 26, Team 1
    ]
  },
)
#set text(size: 10.5pt)
#set par(justify: true)
#set heading(numbering: "1.1")
#show heading.where(level: 1): it => { v(0.6em); it; v(0.2em) }
#show link: set text(fill: rgb("#1a5fb4"))
#show raw.where(block: false): box.with(fill: luma(240), inset: (x: 2pt), outset: (y: 2pt), radius: 2pt)
#show raw.where(block: true): block.with(fill: luma(245), inset: 8pt, radius: 3pt, width: 100%)
#set table(
  stroke: 0.5pt + luma(180),
  inset: 6pt,
  fill: (_, y) => if y == 0 { luma(230) },
)
#show table.cell.where(y: 0): strong

#let note(body) = block(
  fill: rgb("#fff6d5"), stroke: (left: 3pt + rgb("#e5a50a")),
  inset: 8pt, width: 100%, body,
)
#let stub(body) = box(
  fill: rgb("#fde2e1"), inset: (x: 4pt, y: 2pt), radius: 2pt,
  text(size: 8.5pt, fill: rgb("#a51d2d"), body),
)

#align(center)[
  #text(size: 22pt, weight: "bold")[RobotLink Protocol]
  #v(-0.4em)
  #text(size: 12pt)[Laptop #sym.arrow.l.r Robot message format, schema v1]
  #v(0.2em)
  #text(size: 10pt, fill: luma(90))[AUR Training 26, Team 1 #sym.dot.c Firmware branch]
]

#v(1em)
#outline(indent: auto, depth: 2)
#pagebreak()

= Introduction

This document describes how the laptop talks to the robot: the transport, every message in both directions, the rules the robot applies to them, and what each drive mode does. It is the reference for anyone writing laptop code. Most of the details are handled for you by `tools/robot_link.py` (see @robotlink); read this to understand what the robot does with your commands and what it reports back.

The schema version is defined as `PROTOCOL_VERSION` in `src/SERVICE/PROTOCOL/PROTOCOL_cfg.h`. Firmware internals (tasks, drivers, layers) are covered separately in `docs/firmware.typ`.

= Connection

#table(
  columns: (auto, 1fr),
  table.header[Item][Value],
  [WiFi network], [The team router's 2.4 GHz network (the ESP32 has no 5 GHz radio). SSID and password are in `src/SERVICE/UDP/UDP_cfg.h`. The robot, the laptop and the camera phone all join it.],
  [Robot address], [`192.168.69.50`, static (`UDP_STATIC_IP`), UDP port `5005` (`UDP_LOCAL_PORT`). Set `UDP_USE_STATIC_IP` to 0 to use DHCP instead; the IP is printed on the serial monitor either way.],
  [Format], [One JSON object per UDP datagram, at most 1472 bytes (one 1500-byte Ethernet frame).],
  [Units], [SI everywhere: m, rad, s, m/s, rad/s.],
  [Arena frame], [Origin at the bottom-left corner, $x$ #sym.arrow EAST, $y$ #sym.arrow NORTH. See @arena.],
)

The robot replies to whichever address sent it the *last valid packet*, so *the laptop always talks first*. Until then the robot sends nothing.

UDP can lose packets. Anything that must arrive (`mode`, `gripper` and `square_ref`) is acknowledged by the robot and retried by the laptop. Everything else is streamed, so a lost packet is simply replaced by the next one.

= Common fields

Every message, in both directions, has these three fields:

#table(
  columns: (auto, auto, 1fr),
  table.header[Field][Type][Meaning],
  [`type`], [string], [Message type (tables below).],
  [`seq`], [integer, 0 to 2#super[32]#sym.minus 1], [Sender's message counter. Must increase with every packet.],
  [`t`], [integer, 0 to 2#super[32]#sym.minus 1], [Sender's clock in ms, for timestamping and round-trip time.],
)

On the robot, `t` is `millis()` since boot. Each direction has its own `seq` counter; the robot's counter is shared by all of its outgoing messages.

= Laptop #sym.arrow robot

#table(
  columns: (auto, 1fr, auto),
  table.header[`type`][Extra fields][When to send],
  [`mode`], [`mode`, plus that mode's parameters (@modes)], [On change. Robot replies `ack`.],
  [`twist`], [`v` (m/s), `omega` (rad/s, + = counter-clockwise)], [50 Hz while in TWIST.],
  [`wheel_vel`], [`wl`, `wr` (rad/s) #stub[not implemented]], [50 Hz while in WHEEL_VEL.],
  [`gripper`], [`lift` (`"UP"`/`"DOWN"`), `grip` (`"OPEN"`/`"CLOSED"`). Each optional, at least one required.], [On change. Robot replies `ack` (@gripper).],
  [`square_ref`], [`side` (`"NORTH"`/`"EAST"`/`"SOUTH"`/`"WEST"`): the edge SQUARE just aligned to], [When SQUARE is in phase `WAITING`. Robot replies `ack` (@square).],
  [`heartbeat`], [none], [50 Hz whenever no stream above is being sent.],
  [`ping`], [none], [Any time. Robot replies `pong`.],
)

Examples:
```json
{"type":"mode","seq":101,"t":5230,"mode":"MOVE","distance":0.5,"v_max":0.3}
{"type":"twist","seq":102,"t":5250,"v":0.2,"omega":0.0}
{"type":"heartbeat","seq":103,"t":5270}
{"type":"gripper","seq":104,"t":5290,"lift":"DOWN","grip":"CLOSED"}
{"type":"square_ref","seq":105,"t":5310,"side":"SOUTH"}
{"type":"ping","seq":106,"t":5330}
```

A `twist` received outside TWIST, or a `wheel_vel` in any mode, is accepted (it counts in `rx_ok` and feeds the watchdog) but otherwise ignored.

= Robot #sym.arrow laptop

#table(
  columns: (auto, 1fr, auto),
  table.header[`type`][Fields][Rate],
  [`odometry`], [`x`, `y` (m), `theta` (rad), `v` (m/s), `omega` (rad/s)], [20 Hz],
  [`sensors`], [`range_m`: ultrasonic range in m, or `null` with no reading. `ir_left`, `ir_right`: `true` while that front-corner IR sees the boundary line; `null` until the IR sensors are read.], [10 Hz],
  [`status`], [`mode`, `phase`, `result`, `reason`, `cmd_seq`, `lift`, `grip`, `watchdog`, `rx_ok`, `rx_bad`], [5 Hz, plus immediately on any change],
  [`ack`], [`ack_seq`, `ok`, `err`], [Reply to each `mode`, `gripper` and `square_ref`],
  [`pong`], [`ping_t`: the `t` of the ping], [Reply to each `ping`],
)

#note[
  *Current data.* The pose in `odometry` is not integrated from the encoders yet: it starts at the boot estimate $(0.15, 0.15, pi/2)$ and only changes when a `square_ref` corrects it. `v` and `omega` are always 0. In `sensors`, `range_m` is always `null` until the ultrasonic driver exists, and `ir_left` / `ir_right` until the IR driver exists.
]

== `status` fields

#table(
  columns: (auto, 1fr),
  table.header[Field][Meaning],
  [`mode`], [Current mode.],
  [`phase`], [What the mode is doing right now, e.g. `IDLE`, `DRIVING`, `NO_INPUT`, `STARTING`, `WAITING`. `STARTING` is shown between a MOVE/TURN/SQUARE command and the first phase the control loop reports.],
  [`result`], [`RUNNING`, `DONE`, `FAILED` or `PREEMPTED`.],
  [`reason`], [Why it failed, e.g. `watchdog`, `not_implemented`, `missing_distance`. Empty otherwise. Each mode lists its reasons in @modes.],
  [`cmd_seq`], [`seq` of the `mode` command this result belongs to, so you can tell which command finished. 0 at boot.],
  [`lift`, `grip`], [The gripper's current commanded state (`UP`/`DOWN`, `OPEN`/`CLOSED`).],
  [`watchdog`], [`true` while no valid packet has arrived for 300 ms.],
  [`rx_ok`, `rx_bad`], [Packets accepted / rejected since boot.],
)

"Any change" means a change in `mode`, `phase`, `result`, `reason`, `cmd_seq`, `lift`, `grip` or `watchdog`; the packet counters alone don't trigger an extra status.

== `ack` fields

`ack_seq` is the `seq` of the command being answered. `ok` is `false` when the robot refused the command, and `err` says why; on success `err` is `""`.

#table(
  columns: (auto, 1fr),
  table.header[`err`][Meaning],
  [`not_implemented`], [A `mode` command for WHEEL_VEL or APPROACH, which aren't built yet.],
  [`not_waiting`], [A `square_ref` sent while SQUARE isn't running in phase `WAITING`.],
)

Examples:
```json
{"type":"status","seq":88,"t":91200,"mode":"TWIST","phase":"DRIVING","result":"RUNNING","reason":"","cmd_seq":101,"lift":"UP","grip":"OPEN","watchdog":false,"rx_ok":412,"rx_bad":0}
{"type":"ack","seq":89,"t":91210,"ack_seq":101,"ok":true,"err":""}
{"type":"odometry","seq":90,"t":91230,"x":0.15,"y":0.1,"theta":-1.5707964,"v":0,"omega":0}
{"type":"sensors","seq":91,"t":91240,"range_m":null,"ir_left":null,"ir_right":null}
{"type":"pong","seq":92,"t":91250,"ping_t":5330}
```

= Rules <rules>

/ Validation: The robot rejects a packet that isn't a JSON object, has an unknown `type`, is missing a required field, or has a required field of the wrong type. A rejected packet counts in `rx_bad` and is otherwise ignored, with *no reply*. The reason is printed on the robot's serial monitor (@rejects).

/ Optional parameters: Optional numeric `mode` parameters of the wrong type (e.g. `"distance":"0.5"`) are treated as absent rather than rejected. An unknown `facing` value rejects the whole packet (`bad_mode`). Unknown extra fields are ignored.

/ Sequence numbers: The robot drops a packet whose `seq` is not higher than the last one it accepted (`old_seq`). Once the watchdog has tripped, any `seq` is accepted, so a restarted script can reconnect. RobotLink starts `seq` from the wall clock, so restarts are accepted right away.

/ Watchdog: Any valid packet keeps the link alive. After *300 ms* with no valid packet, the robot switches to STOP and reports `result: FAILED`, `reason: watchdog`. It stays in STOP until it receives a new `mode` command. The gripper is *not* affected: it holds its state.

/ Stream staleness: In TWIST, if no `twist` arrives for 300 ms, the robot sets $v$ and $omega$ to 0 (`phase: NO_INPUT`), even if heartbeats keep the link alive.

/ Edge guard: In every mode except SQUARE, a wheel whose front-corner IR sees the boundary line can't drive forward; it can still stop or reverse. Backing off and turning away work normally, and pushing straight on makes the robot pivot square to the edge instead of crossing it. Watch `sensors.ir_left` / `ir_right` to see when it's acting. Limits:
  - *Forward only:* there are no rear sensors, so reversing over a line isn't caught.
  - *No override.*
  - *Not active yet:* it switches on once the IR sensors are read (`ir_*` no longer `null`).

/ Preemption: A new `mode` command replaces the current one immediately. The old command first reports `PREEMPTED` with its own `cmd_seq`.

/ Repeats: Sending the mode the robot is already running (e.g. a retry after a lost ack) is acknowledged and changes nothing: `cmd_seq` stays that of the command that started the mode, so the mode isn't restarted.

/ End of a mode: When a mode reports DONE or FAILED, the robot returns to STOP, keeping that command's `cmd_seq`, `result` and `reason` in the status. Sending STOP afterwards starts a fresh STOP with a new `cmd_seq` and `result: RUNNING`.

/ Entering a mode: The robot resets its controllers and takes its current position and heading as the starting point.

== Reject reasons <rejects>

These are never sent to the laptop. They appear on the robot's serial monitor (115200 baud) as `rejected packet: <reason> (rx_bad=N)`.

#table(
  columns: (auto, 1fr),
  table.header[Reason][Cause],
  [`bad_json`], [Not parseable as JSON.],
  [`not_object`], [Valid JSON, but not an object.],
  [`bad_header`], [`type` is not a string, or `seq` / `t` is missing or not an unsigned 32-bit integer.],
  [`unknown_type`], [`type` isn't one of the laptop #sym.arrow robot types.],
  [`bad_twist`], [`v` or `omega` missing or not a number.],
  [`bad_wheel_vel`], [`wl` or `wr` missing or not a number.],
  [`bad_mode`], [`mode` missing or unknown, or `facing` present but not a valid side.],
  [`bad_gripper`], [Neither `lift` nor `grip` present, or one present with an unknown state.],
  [`bad_square_ref`], [`side` missing or not a valid side.],
  [`old_seq`], [`seq` not higher than the last accepted one, while the link is alive.],
)

== Timing constants

#table(
  columns: (auto, auto, 1fr),
  table.header[Constant][Value][Defined in],
  [Watchdog timeout], [300 ms], [`COMM_WATCHDOG_MS`, `src/SERVICE/COMM/COMM_cfg.h`],
  [Twist staleness], [300 ms], [`COMMTASK_TWIST_STALE_MS`, `src/APP/COMM_TASK/COMM_TASK_cfg.h`],
  [Odometry period], [50 ms (20 Hz)], [`COMM_ODOMETRY_PERIOD_MS`],
  [Sensors period], [100 ms (10 Hz)], [`COMM_SENSORS_PERIOD_MS`],
  [Status period], [200 ms (5 Hz) + on change], [`COMM_STATUS_PERIOD_MS`],
  [Robot packet polling], [every 1 ms], [`COMMTASK_PERIOD_MS`],
  [Largest robot message], [320 bytes], [`PROTOCOL_TX_BUF_SIZE`],
)

= Arena frame <arena>

The robot keeps its own pose in the arena and reports it in `odometry`:

- *Origin:* the bottom-left (south-west) corner of the square arena.
- *Axes:* $x$ points EAST, $y$ points NORTH, in meters.
- *Heading:* $theta$ is measured from EAST, counter-clockwise positive. Facing EAST is 0, NORTH is $+pi/2$, WEST is $pi$, SOUTH is $-pi/2$.

#align(center, box(width: 6.5cm, height: 6.5cm, {
  place(dx: 0.5cm, dy: 0.5cm, rect(width: 5.5cm, height: 5.5cm, stroke: 1pt))
  place(dx: 2.6cm, dy: 0cm, text(size: 9pt)[NORTH])
  place(dx: 2.6cm, dy: 6.1cm, text(size: 9pt)[SOUTH])
  place(dx: 6.1cm, dy: 3.1cm, text(size: 9pt)[E])
  place(dx: 0cm, dy: 3.1cm, text(size: 9pt)[W])
  place(dx: 0.5cm, dy: 6cm, line(length: 1.6cm, stroke: 1.5pt + rgb("#c01c28")))
  place(dx: 2.15cm, dy: 5.75cm, text(size: 9pt, fill: rgb("#c01c28"))[$x$])
  place(dx: 0.5cm, dy: 4.4cm, line(angle: 90deg, length: 1.6cm, stroke: 1.5pt + rgb("#26a269")))
  place(dx: 0.65cm, dy: 4.1cm, text(size: 9pt, fill: rgb("#26a269"))[$y$])
  place(dx: 0.55cm, dy: 6.05cm, text(size: 8pt)[$(0,0)$])
}))

At boot the pose is a rough estimate (near the bottom-left corner, facing NORTH). It becomes exact by squaring on the arena edges (@square). Squaring is only done on the four boundary edges.

*Localizing at the start* (from the bottom-left corner):
+ Face the SOUTH edge #sym.arrow `set_mode("SQUARE")` #sym.arrow wait for phase `WAITING` #sym.arrow `set_square_side("SOUTH")`. $theta$ and $y$ are now exact.
+ `set_mode("TURN", facing="WEST")` #sym.arrow SQUARE #sym.arrow wait for `WAITING` #sym.arrow `set_square_side("WEST")`. $x$ is now exact too.

Square on any edge again later to remove odometry drift.

= Modes <modes>

#table(
  columns: (auto, auto, 1fr),
  table.header[Mode][Accepted?][Current state],
  [STOP], [yes], [Fully implemented in the protocol layer (motor output pending).],
  [TWIST], [yes], [Fully implemented in the protocol layer; wheel targets computed (motor output pending).],
  [MOVE], [yes], [Stub: immediately reports FAILED and returns to STOP.],
  [TURN], [yes], [Stub: immediately reports FAILED and returns to STOP.],
  [SQUARE], [yes], [Driving phases are a stub: goes straight to `WAITING`. From there it works: `square_ref` corrects the pose and SQUARE ends DONE.],
  [WHEEL_VEL], [no], [Acked `ok: false`, `err: "not_implemented"`.],
  [APPROACH], [no], [Acked `ok: false`, `err: "not_implemented"`.],
)

Parameters marked `?` are optional. The laptop sends them as extra fields of the `mode` message.

== STOP

The safe resting state. The robot boots into STOP, and returns to it after DONE, FAILED or a watchdog trip.
- *Parameters:* none.
- *Behavior:* ramps the speed down to zero, then releases the motors.
- *Phases:* `IDLE`.
- *Ends:* only when a new `mode` command arrives.

== TWIST

Continuous driving from the laptop, e.g. a joystick or your own path controller.
- *Parameters:* none. Then stream `twist` at 50 Hz.
- *Behavior:*
  - follows `v` and `omega`, with acceleration limits;
  - when `omega = 0`, holds the current heading using the gyro, so "straight" stays straight;
  - the edge guard (@rules) stops a wheel driving forward over the boundary line.
- *Phases:* `NO_INPUT` (no fresh twist; stopped), `DRIVING`.
- *Ends:* never on its own.

== WHEEL_VEL #stub[not implemented]

Testing only: commands each wheel directly. Used to characterize the motors and tune the wheel controllers.
- *Parameters:* none. Then stream `wheel_vel` at 50 Hz.
- *Ends:* never on its own. If the stream stops, both wheels go to 0.

== MOVE #stub[stub reports not_implemented]

Drive a set distance in a straight line.
- *Parameters:* `distance` (m, negative = backwards), `v_max?` (m/s), `a_max?` (m/s#super[2]).
- *Behavior:* a smooth speed profile over the distance (measured by the wheel encoders), while holding the starting heading (gyro).
- *Phases:* `DRIVING`.
- *DONE:* within 1 cm of the target and stopped.
- *FAILED:*
  - `missing_distance`: no `distance` given;
  - `timeout`: took more than twice the planned time;
  - `stall`: motors at full power but not moving;
  - `obstacle`: the ultrasonic sensor saw something too close ahead;
  - `not_implemented`: what the stub reports when `distance` is given.

== TURN #stub[stub reports not_implemented]

Rotate in place, either by an angle or to face an arena side.
- *Parameters:* exactly one of:
  - `angle`: rad, relative, + = counter-clockwise;
  - `facing`: `"NORTH"`/`"EAST"`/`"SOUTH"`/`"WEST"`, absolute in the arena frame. Meant for the D-pad.

  Optional: `omega_max?` (rad/s), `alpha_max?` (rad/s#super[2]).
- *Behavior:* a smooth turn-rate profile (measured by the gyro), while preventing forward creep.
- *Phases:* `TURNING`.
- *DONE:* within 1° of the target and stopped.
- *FAILED:*
  - `missing_angle`: neither `angle` nor `facing` given;
  - `conflicting_target`: both given;
  - `no_heading_ref`: `facing` used before any `square_ref` since boot, so the robot doesn't know where NORTH is yet;
  - `timeout`, `stall`;
  - `not_implemented`: what the stub reports when the target is valid.

== SQUARE <square>

#stub[driving phases are a stub; goes straight to WAITING]

Drive slowly forward to an arena edge line and align square to it, using the two IR line sensors on the front corners. Then the driver says which edge it was, and the robot corrects its pose. The edge guard is off in SQUARE, since it drives onto the line on purpose.
- *Parameters:* `max_search` (m), `v_search?` (m/s).
- *Phases:*
  + `SEARCH`: driving forward.
  + `FIRST_TRIP`: one sensor has hit the line.
  + `SECOND_TRIP`: both sensors have hit; the angle is computed.
  + `ALIGN`: turning square.
  + `WAITING`: stopped, waiting for `square_ref`. *There's no timeout.* To cancel, send any other mode (the robot reports PREEMPTED and the pose is unchanged).
- *`square_ref`:* only accepted in `WAITING`, otherwise it's acked `ok: false`, `err: "not_waiting"`. The robot is facing the named edge with its IR sensors on the line, so it sets ($S$ = arena size, $d$ = IR sensor offset ahead of the robot's center):

  #table(
    columns: 3,
    table.header[`side`][`theta`][Coordinate],
    [NORTH], [$+pi/2$], [$y = S - d$],
    [SOUTH], [$-pi/2$], [$y = d$],
    [EAST], [$0$], [$x = S - d$],
    [WEST], [$pi$], [$x = d$],
  )

  The other coordinate is unchanged. Then SQUARE ends with DONE. If the ack of a `square_ref` is lost, the retry gets `not_waiting` (SQUARE has already finished), so check the status for `result: DONE`, not only the ack.

  $S$ and $d$ are `CONTROLTASK_ARENA_SIZE_M` (currently 3.0 m) and `CONTROLTASK_IR_FORWARD_OFFSET_M` (currently 0.10 m) in `src/APP/CONTROL_TASK/CONTROL_TASK_cfg.h`. Both are placeholders until measured.
- *DONE:* aligned and the pose corrected.
- *FAILED:*
  - `no_line`: no line within `max_search`;
  - `one_side_only`: the second sensor never hit, meaning the line is too steep or has a gap;
  - `timeout`.

== APPROACH #stub[not implemented]

Close in on a box to grabbing distance, using the ultrasonic sensor.
- *Parameters:* `grab_distance` (m), `v_max?` (m/s).
- *Behavior:* slows smoothly to stop at `grab_distance`, holding heading. Optionally the laptop can steer it with a camera bearing; that message will be added when this mode is built.
- *DONE:* within 1 cm of `grab_distance` and stopped.
- *FAILED:*
  - `lost_target`: no ultrasonic echo for several readings;
  - `max_travel`: drove too far without arriving;
  - `timeout`.

= Gripper <gripper>

Two servos, commanded independently of the drive mode with a `gripper` message:
- *`lift`:* `UP` or `DOWN` (the angle servo).
- *`grip`:* `OPEN` or `CLOSED` (the state servo).

Rules:
- *Each field is optional.* Leave one out to keep that servo where it is. A message with neither, or with an unknown state, is rejected (`bad_gripper`, no ack).
- *It works in any mode*, including while driving in TWIST, and never changes the drive mode.
- *It holds through mode changes and watchdog trips,* so a WiFi blip doesn't drop a box.
- *At boot* the robot assumes `UP` / `OPEN`.
- *Current status: mock.* The robot acks, logs and reports the state, but the servos aren't driven yet. The robot maps each state to a servo angle itself, so laptop code never deals with angles.

= Example exchanges

== Starting TWIST and losing the link

#table(
  columns: (auto, auto, 1fr),
  table.header[Direction][Message][Effect],
  [L #sym.arrow R], [`mode` seq 101 `"TWIST"`], [Robot enters TWIST, phase `NO_INPUT`.],
  [R #sym.arrow L], [`ack` `ack_seq` 101, `ok: true`], [],
  [R #sym.arrow L], [`status` TWIST / NO_INPUT / RUNNING, `cmd_seq` 101], [Sent immediately (changed).],
  [L #sym.arrow R], [`twist` 50 Hz], [Phase becomes `DRIVING`.],
  [(laptop crashes)], [], [],
  [R #sym.arrow L], [`status` STOP / IDLE / FAILED / `watchdog`, `cmd_seq` 101], [300 ms after the last packet.],
)

== Preempting a running mode

#table(
  columns: (auto, auto, 1fr),
  table.header[Direction][Message][Effect],
  [L #sym.arrow R], [`mode` seq 200 `"SQUARE"`, `max_search` 0.5], [SQUARE starts, `STARTING` then `WAITING`.],
  [L #sym.arrow R], [`mode` seq 230 `"TWIST"`], [Cancels SQUARE.],
  [R #sym.arrow L], [`status` SQUARE / PREEMPTED, `cmd_seq` 200], [The old command's result.],
  [R #sym.arrow L], [`ack` `ack_seq` 230, `ok: true`], [],
  [R #sym.arrow L], [`status` TWIST / NO_INPUT / RUNNING, `cmd_seq` 230], [],
)

= Using RobotLink <robotlink>

`tools/robot_link.py` needs Python 3.10+. It:
- sends heartbeats or your twist at 50 Hz in a background thread;
- fills in `seq` and `t`;
- retries `mode`, `gripper` and `square_ref` commands until they're acknowledged (3 tries, 0.3 s each, a new `seq` per try);
- keeps the latest odometry, sensors and status for you;
- ignores packets from any address other than the robot's.

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
    print(robot.latest_sensors())          # range_m, ir_left, ir_right: None until the drivers exist

    # Localize: square on the SOUTH edge
    robot.set_mode("SQUARE", max_search=0.5)
    while (robot.latest_status() or {}).get("phase") != "WAITING":
        time.sleep(0.01)
    robot.set_square_side("SOUTH")         # theta and y corrected in the next odometry
    robot.set_mode("TURN", facing="WEST")  # D-pad: face an arena side

    ok, err = robot.set_mode("MOVE", distance=0.5)   # acked, then status: FAILED/not_implemented (stub)
    robot.set_mode("STOP")
```

#table(
  columns: (auto, 1fr),
  table.header[Method][Description],
  [`RobotLink(host, port, rate_hz)`], [Defaults: `192.168.69.50`, `5005`, 50 Hz. Use as a context manager, or call `start()` / `close()`.],
  [`set_mode(mode, retries, timeout, **params)`], [Sends `mode` and waits for the ack. Returns `(ok, err)`. Stops twist streaming first; restarts it (at 0, 0) if TWIST is accepted. Raises `ValueError` for an unknown mode or `facing`.],
  [`set_twist(v, omega)`], [Sets the twist streamed while in TWIST.],
  [`set_gripper(lift, grip, ...)`], [Sends `gripper`, waits for the ack. Raises `ValueError` if both are `None` or a state is unknown.],
  [`set_square_side(side, ...)`], [Sends `square_ref`, waits for the ack.],
  [`ping(timeout)`], [Round-trip time in ms, or `None`.],
  [`latest_odometry()`, `latest_sensors()`, `latest_status()`], [Copies of the newest messages, or `None`.],
)

To check on a command, compare `latest_status()["cmd_seq"]` with the command you sent and look at its `result`. When your script exits, the background stream stops, and the robot's watchdog brings it to STOP within 300 ms.

Test scripts using RobotLink live next to it in `tools/`: `ping_test.py` (RTT and loss), `heartbeat_test.py` (modes, telemetry rates, watchdog timing) and `mock_test.py` (gripper, squaring, MOVE/TURN stubs, sensors).
