// Firmware documentation for the AUR Training 26 Team 1 robot.
// Compile with: typst compile docs/firmware.typ

#set document(title: "Robot Firmware", author: "AUR Training 26, Team 1")
#set page(
  paper: "a4",
  margin: (x: 2cm, y: 2.2cm),
  numbering: "1",
  header: context {
    if counter(page).get().first() > 1 [
      #set text(size: 8.5pt, fill: luma(110))
      Robot Firmware #h(1fr) AUR Training 26, Team 1
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
#let warn(body) = block(
  fill: rgb("#fde2e1"), stroke: (left: 3pt + rgb("#c01c28")),
  inset: 8pt, width: 100%, body,
)
#let tag(color, body) = box(
  fill: color.lighten(80%), inset: (x: 4pt, y: 2pt), radius: 2pt,
  text(size: 8.5pt, fill: color.darken(30%), weight: "bold", body),
)
#let done = tag(rgb("#26a269"))[done]
#let mock = tag(rgb("#e5a50a"))[mock]
#let todo = tag(rgb("#c01c28"))[todo]

#align(center)[
  #text(size: 22pt, weight: "bold")[Robot Firmware]
  #v(-0.4em)
  #text(size: 12pt)[ESP32 firmware for the AUR Training 26 final project]
  #v(0.2em)
  #text(size: 10pt, fill: luma(90))[AUR Training 26, Team 1 #sym.dot.c `firmware` branch]
]

#v(1em)
#outline(indent: auto, depth: 2)
#pagebreak()

= Overview

The robot is a two-wheeled differential-drive platform built on an ESP32 (`esp32dev`). It drives in a square arena, picks up boxes with a two-servo gripper, and is controlled from a laptop over WiFi. The laptop does the high-level work (camera, planning, operator input) and sends commands; the robot runs the real-time loops, keeps its own pose estimate, and streams telemetry back.

The firmware is written for the Arduino framework on top of ESP-IDF and FreeRTOS, and is split into four layers (@layers). Two FreeRTOS tasks do all the work at runtime: a *comms task* that owns the WiFi link and the protocol, and a *control task* that owns every driver and the control loops (@runtime).

The laptop #sym.arrow.l.r robot message format is documented separately in `docs/protocol.typ`. This document covers how the firmware is built and how it works inside.

== Implementation status

#table(
  columns: (auto, auto, 1fr),
  table.header[Area][State][Notes],
  [WiFi + UDP link], done, [Station mode, static IP, auto-reconnect.],
  [JSON protocol, watchdog, acks], done, [Schema v1, all message types decoded.],
  [Mode state machine (comms side)], done, [Preemption, repeats, DONE/FAILED #sym.arrow STOP.],
  [Telemetry streaming], done, [Odometry 20 Hz, sensors 10 Hz, status 5 Hz + on change.],
  [TWIST inverse kinematics], done, [Wheel targets computed; not sent to motors yet.],
  [Edge guard], done, [Wired in; inactive until the IR sensors are read.],
  [SQUARE pose correction], mock, [Driving phases skipped; `square_ref` correction is real.],
  [MOVE, TURN], mock, [Validate parameters, then report FAILED/`not_implemented`.],
  [Gripper], mock, [State tracked and reported; servos not driven.],
  [MPU6050 driver (HAL)], done, [Init + burst read; not called from the control loop yet.],
  [Encoder driver (HAL, PCNT)], done, [4x quadrature; counts not read by the control loop yet.],
  [GPIO, I2C, PWM drivers (MCAL)], done, [Available; GPIO and PWM not used yet.],
  [Odometry integration], todo, [Pose only changes on `square_ref`.],
  [Wheel speed loops, motor output], todo, [Motor pins and driver TBD.],
  [Ultrasonic driver], todo, [Pins configured in `main2.cpp` only.],
  [IR line sensor reading], todo, [Pins TBD.],
  [WHEEL_VEL, APPROACH modes], todo, [Acked `not_implemented`.],
)

= Building and flashing

The project is a PlatformIO project (`platformio.ini`):

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps = bblanchon/ArduinoJson@^7
```

The only external library is ArduinoJson 7, which PlatformIO downloads on the first build.

```sh
pio run                     # build
pio run -t upload           # flash
pio device monitor          # serial log, 115200 baud
```

Before flashing, set the router's SSID, password and the robot's static IP in `src/SERVICE/UDP/UDP_cfg.h` (@cfg-udp). The serial monitor prints the IP and every mode change, rejected packet and watchdog event (@serial-log), so keep it open while testing.

== Repository layout

```
platformio.ini            PlatformIO build configuration
docs/
  firmware.typ            this document
  protocol.typ            laptop <-> robot message format
  protocol.md             older Markdown version of the protocol
src/
  main2.cpp               entry point: setup() / loop()
  APP/
    COMM_TASK/            FreeRTOS task: link, protocol, mode state machine
    CONTROL_TASK/         FreeRTOS task: drivers, odometry, control loops
  SERVICE/
    UDP/                  WiFi station + UDP socket (UdpLink)
    PROTOCOL/             JSON encode/decode (Protocol)
    COMM/                 UdpLink + Protocol + watchdog (Comms)
  HAL/
    MPU6050/              IMU driver over I2C
    PCNT/                 quadrature encoder driver over the PCNT peripheral
  MCAL/
    GPIO/                 digital pins (register access)
    I2C/                  I2C master
    PWM/                  LEDC timers/channels, servo helper
  AUR_finalProject.code-workspace   VS Code workspace (not needed to build)
tools/
  robot_link.py           laptop-side client library
  ping_test.py            RTT / loss test
  heartbeat_test.py       modes, telemetry rates, watchdog test
  mock_test.py            gripper, squaring, MOVE/TURN stub test
include/, lib/, test/     PlatformIO defaults (unused)
```

= Architecture <layers>

== Layers

Each layer may only call the layers below it.

#let layer(name, color, desc, mods) = rect(
  width: 100%, fill: color.lighten(85%), stroke: 0.8pt + color, radius: 3pt, inset: 8pt,
  grid(columns: (3.2cm, 1fr), gutter: 8pt,
    [#text(weight: "bold", fill: color.darken(30%), name)\ #text(size: 8.5pt, desc)],
    align(horizon, text(size: 9.5pt, mods)),
  ),
)

#block(breakable: false, stack(spacing: 4pt,
  layer("APP", rgb("#613583"), [application tasks],
    [`COMM_TASK` (core 0) #h(1em) `CONTROL_TASK` (core 1)]),
  layer("SERVICE", rgb("#1a5fb4"), [communication stack],
    [`COMM` #sym.arrow.r `PROTOCOL` + `UDP` #h(1em) (C++ classes)]),
  layer("HAL", rgb("#26a269"), [device drivers],
    [`MPU6050` (IMU) #h(1em) `PCNT` (wheel encoders)]),
  layer("MCAL", rgb("#c64600"), [microcontroller peripherals],
    [`GPIO` #h(1em) `I2C` #h(1em) `PWM` (LEDC)]),
  rect(width: 100%, fill: luma(235), stroke: 0.8pt + luma(150), radius: 3pt, inset: 8pt,
    align(center, text(size: 9.5pt)[ESP-IDF drivers #sym.dot.c Arduino core (`WiFi`, `WiFiUdp`) #sym.dot.c FreeRTOS #sym.dot.c ArduinoJson])),
))

MCAL and HAL are plain C with `extern "C"` guards. SERVICE and APP are C++ (classes and `enum class`).

== Module file convention

Every module is a folder with three files:

#table(
  columns: (auto, 1fr),
  table.header[File][Contents],
  [`<MOD>_int.h`], [Interface: public types and function prototypes. The only file other modules include.],
  [`<MOD>_cfg.h`], [Configuration: pins, rates, limits, placeholders. Edit this, not the code.],
  [`<MOD>_prg.c` / `.cpp`], [Program: the implementation.],
)

== Naming convention

Functions are named `MODULE_<prefix>Name`, where the prefix gives the return type: `v` void, `b` bool, `x` an enum/struct type, `u8`/`s16` integers, `pc` `const char *`. For example `COMMTASK_bStart`, `I2C_xReadBytes`, `PCNT_s16GetCount`, `PROTOCOL_pcModeName`.

Variables use the same prefixes (`u32LastStatusMs`, `fWl`, `bHaveActive`, `xState`), and parameters start with `Copy_` (passed by value) or `Copy_px` (pointer). The SERVICE classes use lower-camel-case members with a trailing underscore (`rxBuf_`, `lastSeq_`).

= Boot sequence

The entry point is `src/main2.cpp`.

```cpp
void setup() {
  Serial.begin(115200);
  delay(200);
  vInitSensors();          // ultrasonic pins, I2C + MPU6050, encoders
  COMMTASK_bStart();       // mailboxes, WiFi join, comms task on core 0
  CONTROLTASK_bStart();    // boot pose, control task on core 1
}

void loop() {
  vTaskDelete(NULL);       // all work happens in tasks
}
```

+ *`vInitSensors()`*
  - Sets the ultrasonic TRIG (GPIO 26) as an output held low and ECHO (GPIO 27) as an input.
  - Starts I2C port 0 as master (SDA 21, SCL 22, 400 kHz).
  - Reads the MPU6050's `WHO_AM_I` register; if it reads `0x68`, wakes the sensor with `MPU6050_vInit()`. Every failure is printed but doesn't stop the boot.
  - Configures PCNT unit 0 for the left encoder (GPIO 34/35) and unit 1 for the right one (GPIO 36/39).
+ *`COMMTASK_bStart()`* creates the five mailboxes (@mailboxes), seeds them with STOP and the boot gripper state, joins the router and opens the UDP socket, then starts the comms task. Joining the router waits up to `UDP_CONNECT_TIMEOUT_MS` (10 s); if WiFi isn't up by then the boot carries on and the link keeps retrying in the background.
+ *`CONTROLTASK_bStart()`* sets the boot pose ($x = 0.15$, $y = 0.15$, facing NORTH) and starts the control task.
+ *`loop()`* deletes the Arduino loop task, since nothing runs there.

#note[Because step 2 can block for up to 10 s, the control task starts late when the router isn't reachable. The robot is stopped during that time anyway.]

= Runtime <runtime>

== Tasks

#table(
  columns: (auto, auto, auto, auto, auto, 1fr),
  table.header[Task][Core][Priority][Period][Stack][Owns],
  [`comm`], [0], [5], [1 ms], [6144 B], [`Comms` (WiFi/UDP/protocol), mode state machine, gripper command, all serial logging.],
  [`control`], [1], [10], [1 ms (1 kHz)], [4096 B], [Every driver (motors, encoders, IMU, IR, ultrasonic, servos), odometry, control loops.],
)

Core 0 is shared with the WiFi stack; core 1 is reserved for the control loop, which has the highest priority there so nothing delays its tick. Both tasks run on `vTaskDelayUntil`, so their period doesn't drift. The control task never blocks and never prints.

== Mailboxes <mailboxes>

The tasks share no variables. They exchange data through five *length-1 FreeRTOS queues* used as mailboxes: the writer calls `xQueueOverwrite` and the reader calls `xQueuePeek`. A reader always sees the latest value, never blocks, and can read the same value many times.

#let taskbox(name, sub) = rect(width: 100%, height: 100%, fill: luma(245), stroke: 1pt, radius: 4pt, inset: 8pt,
  align(center + horizon)[*#name*\ #text(size: 8.5pt, sub)])
#let mb(dir, name, type) = block(width: 100%, inset: (y: 3pt),
  align(center, text(size: 9pt)[#if dir == ">" [#sym.arrow.r.long] else [#sym.arrow.l.long] #h(4pt) #raw(name) #h(4pt) #if dir == ">" [#sym.arrow.r.long] else [#sym.arrow.l.long]\ #text(size: 8pt, fill: luma(90), type)]))

#block(breakable: false, grid(columns: (3.4cm, 1fr, 3.4cm), rows: 6.2cm, gutter: 6pt,
  taskbox("comm task", [core 0, prio 5\ 1 ms]),
  align(horizon, stack(spacing: 2pt,
    mb(">", "xMotionMailbox", "MotionCommand_t"),
    mb(">", "xGripperMailbox", "GripperCommand_t"),
    mb("<", "xResultMailbox", "MotionResult_t"),
    mb("<", "xOdometryMailbox", "Odometry"),
    mb("<", "xSensorsMailbox", "Sensors"),
  )),
  taskbox("control task", [core 1, prio 10\ 1 kHz]),
))

The mailboxes are private to `COMM_TASK_prg.cpp`; the control task uses the accessor functions below.

#table(
  columns: (auto, auto, 1fr),
  table.header[Function][Direction][Purpose],
  [`COMMTASK_bGetCommand()`], [comm #sym.arrow control], [Current drive mode, twist ($v$, $omega$), mode parameters, `cmdSeq`, and the pending `square_ref` side.],
  [`COMMTASK_bGetGripper()`], [comm #sym.arrow control], [Commanded lift/grip and the `seq` of the last gripper command.],
  [`COMMTASK_vSetMotionResult()`], [control #sym.arrow comm], [Progress of the current mode: phase, RUNNING/DONE/FAILED, failure reason, tagged with `cmdSeq`.],
  [`COMMTASK_vSetOdometry()`], [control #sym.arrow comm], [Pose and velocity for the `odometry` message.],
  [`COMMTASK_vSetSensors()`], [control #sym.arrow comm], [Range and IR readings for the `sensors` message.],
)

A new command is recognized by a change in `mode` or `cmdSeq`. Results carry the `cmdSeq` they belong to, so the comms task ignores a stale result left over from a previous command. The `phase` and `reason` strings in a result are pointers, so they must be string literals.

= Comms task

Files: `src/APP/COMM_TASK/`. Every millisecond, `vStep()` does the following:

+ *WiFi state.* Logs when WiFi comes up or drops. Drops are only logged: the watchdog handles stopping.
+ *Receive.* Calls `Comms::poll()` until it returns false, dispatching each command:
  - `mode` #sym.arrow `vHandleMode()`
  - `twist` #sym.arrow `vHandleTwist()` (ignored unless in TWIST)
  - `gripper` #sym.arrow `vHandleGripper()`
  - `square_ref` #sym.arrow `vHandleSquareRef()`
  - `wheel_vel` #sym.arrow ignored (not implemented)

  `heartbeat` and `ping` are handled inside `Comms` and never reach the task.
+ *Rejects.* If `rx_bad` went up, logs the reason of the last rejected packet.
+ *Control results.* `vApplyMotionResult()` peeks the result mailbox. For MOVE/TURN/SQUARE, a result with the current `cmdSeq` either updates the phase (RUNNING) or ends the mode (DONE/FAILED #sym.arrow STOP).
+ *Watchdog.* When the link goes from alive to dead, a running mode ends as FAILED/`watchdog`. The gripper is left alone.
+ *Twist staleness.* In TWIST, if no twist arrived in the last 300 ms, $v$ and $omega$ are zeroed and the phase is `NO_INPUT`, otherwise `DRIVING`. The current twist is logged every 500 ms.
+ *Publish the command.* Writes the current `MotionCommand_t` to the motion mailbox.
+ *Telemetry.* Sends `odometry` every 50 ms and `sensors` every 100 ms from the latest mailbox contents, and `status` every 200 ms or as soon as any status field changes.

== Mode state machine

The task keeps one `RobotState_t`: mode, phase, result, reason, `cmdSeq`, parameters, twist and the pending `square_ref`.

#table(
  columns: (auto, 1fr),
  table.header[Event][Handling],
  [`mode` for WHEEL_VEL / APPROACH], [Ack `ok: false`, `not_implemented`. State unchanged.],
  [`mode` for the mode already running], [Ack only. `cmdSeq` is kept, so the control task doesn't restart the mode (a retry after a lost ack).],
  [Any other `mode`], [If a non-STOP mode is still running, publish its status as PREEMPTED first. Then `vEnterMode()`: set the new mode, reset result/reason/twist/`square_ref`, store the parameters and `cmdSeq`, set the initial phase (`IDLE` for STOP, `NO_INPUT` for TWIST, `STARTING` for the rest). Then ack.],
  [`gripper`], [Update the given fields, store the `seq`, overwrite the gripper mailbox, ack.],
  [`square_ref`], [Accepted only while SQUARE is RUNNING in phase `WAITING`; stores the side for the control task and acks. Otherwise acks `not_waiting`.],
  [Control result DONE / FAILED], [`vEndInStop()`: switch to STOP/`IDLE`, keep `cmdSeq`, set result and reason, clear parameters and twist.],
  [Watchdog trip], [`vEndInStop(FAILED, "watchdog")` unless already in STOP.],
)

Which modes take their phase and result from the control task is decided by `bControlOwned()` (MOVE, TURN, SQUARE). STOP and TWIST are managed entirely in the comms task.

== Serial log <serial-log>

All logging happens here, at 115200 baud. Typical lines:

```
Joining "<SSID>"...
WiFi up, IP 192.168.69.50, UDP port 5005
link up
mode -> TWIST (cmd 101)
twist v=0.200 omega=0.000 (DRIVING)
gripper -> lift=DOWN grip=CLOSED (cmd 104)
square_ref -> SOUTH (cmd 105)
mode SQUARE DONE (cmd 200) -> STOP
mode MOVE FAILED/not_implemented (cmd 210) -> STOP
mode APPROACH rejected: not implemented
rejected packet: old_seq (rx_bad=3)
watchdog: link lost
mode TWIST FAILED/watchdog (cmd 101) -> STOP
```

= Control task

Files: `src/APP/CONTROL_TASK/`. Each 1 ms tick runs three steps:

```cpp
vOdometryStep();   // read every sensor, update the pose, publish it
vMotionStep();     // run the current mode, compute wheel targets
vGripperStep();    // act on a new gripper command
```

== Odometry step

Publishes the pose (`xOdom`) and sensor readings (`xSensors`) to the comms task every tick. The pose starts at the boot estimate and is changed only by `vApplySquareRef()` for now. The planned work, marked `TODO` in the code:

- *Encoders:* read both PCNT units, accumulate the 16-bit deltas into 32-bit positions, estimate wheel velocities.
- *Gyro:* read the MPU6050, subtract a calibrated bias (recalibrated while stationary), integrate the $z$ rate into heading.
- *Odometry:* integrate $x$, $y$, $theta$ from the wheel distances and the gyro heading.
- *Ultrasonic:* latest median-filtered range into `rangeM`.
- *IR line sensors:* read both inputs into `irLeft` / `irRight` and set `hasIr`, which also arms the edge guard.

== Motion step

`vMotionStep()` reads the motion mailbox. If the mode or `cmdSeq` changed, it calls `vEnterMode()` (planned: reset the wheel-loop integrators, latch the start pose, initialize slew limiters) and passes `bNew = true` to the mode's step function.

#table(
  columns: (auto, 1fr),
  table.header[Mode][Step function behavior today],
  [STOP (and anything unhandled)], [`vStepStop()`: wheel targets 0. Planned: ramp down through the slew limiter, then coast.],
  [TWIST], [`vStepTwist()`: inverse kinematics to wheel speeds (below). Planned: slew limiting, gyro heading hold at $omega = 0$, desaturation.],
  [MOVE], [`vStepMove()`: on entry, reports FAILED with `missing_distance` (no `distance`) or `not_implemented`.],
  [TURN], [`vStepTurn()`: on entry, reports FAILED with `missing_angle`, `conflicting_target`, `no_heading_ref` (`facing` before any `square_ref`), or `not_implemented`.],
  [SQUARE], [`vStepSquare()`: on entry, reports RUNNING/`WAITING` (driving phases skipped). When a `square_ref` arrives, applies it and reports DONE.],
)

=== TWIST kinematics

With wheel radius $r$ (`CONTROLTASK_WHEEL_RADIUS_M`) and track width $b$ (`CONTROLTASK_TRACK_WIDTH_M`), the wheel speed targets in rad/s (+ = forward) are:

$ omega_l = (v - omega b / 2) / r, quad omega_r = (v + omega b / 2) / r $

=== Edge guard

Every wheel target goes through `vSetWheelTargets()`, so no mode can bypass the guard. Outside SQUARE, and once `hasIr` is true, a wheel whose IR sensor sees the line has a positive (forward) target clamped to 0; zero and negative targets pass through. With one wheel held, pushing on makes the robot pivot square to the edge. There are no rear sensors, so reversing isn't guarded. The motor output after the guard is still a `TODO` (feed-forward + PI per wheel, duty $= V / V_"batt"$).

=== SQUARE pose correction

`vApplySquareRef(side)` assumes the robot faces that edge with its IR sensor line on it. With arena size $S$ and IR offset $d$:

#table(
  columns: 3,
  table.header[Side][$theta$][Coordinate set],
  [NORTH], [$pi/2$], [$y = S - d$],
  [SOUTH], [$-pi/2$], [$y = d$],
  [EAST], [$0$], [$x = S - d$],
  [WEST], [$pi$], [$x = d$],
)

It also sets `bHaveHeadingRef`, which TURN `facing` requires. Once odometry is integrated, the gyro heading and encoder odometry must be rebased here too (`TODO`).

The planned real SQUARE runs `SEARCH` #sym.arrow `FIRST_TRIP` #sym.arrow `SECOND_TRIP` #sym.arrow `ALIGN` #sym.arrow `WAITING`, computing the misalignment as $theta_"err" = arctan(Delta s \/ w)$ from the distance $Delta s$ driven between the two IR trips and the IR spacing $w$.

=== Planned MOVE and TURN

Documented in the code as `TODO` blocks:
- *MOVE:* trapezoidal profile on distance giving $d_"ref"$ and $v_"ff"$; $v_"cmd" = v_"ff" + K_d (d_"ref" - d_"enc")$; gyro heading hold; DONE within 1 cm and stopped for 100 ms; FAILED on `timeout` (> 2#sym.times planned time), `stall`, `obstacle`.
- *TURN:* trapezoidal profile on the wrapped heading error; $omega_"cmd" = omega_"ff" + K_theta "wrap"(theta_"ref" - theta_"gyro")$; distance loop holding 0; DONE within 1° and stopped for 100 ms; FAILED on `timeout`, `stall`.

== Gripper step

Acts once per new gripper `cmdSeq`. Mapping lift/grip to servo angles and calling `PWM_vSetServoAngle()` is a `TODO`; the servo pins and angles aren't defined yet.

= Service layer

== UDP link (`SERVICE/UDP`)

`UdpLink` moves bytes between the ESP32 and the laptop and knows nothing about their contents.

#table(
  columns: (auto, 1fr),
  table.header[Method][Description],
  [`begin()`], [Station mode, modem sleep off (it adds tens of ms of latency), auto-reconnect, optional static IP; waits up to 10 s for the router; binds port 5005. Returns false only if the IP config or the socket bind fails.],
  [`receive(buf, cap)`], [Non-blocking. Copies the next datagram and remembers its sender as *untrusted*. Returns 0 if nothing is waiting or the datagram is larger than `cap`.],
  [`acceptSender()`], [Promotes the last sender to peer and resets the watchdog timer. Called only after the packet is validated, so garbage can't hijack the link.],
  [`send(data, len)`], [Sends to the peer; does nothing until a peer exists.],
  [`msSinceValidRx()`], [Time since the last accepted packet, or `UINT32_MAX` if none yet.],
  [`wifiConnected()`, `localIp()`], [WiFi state and the robot's address.],
)

== Protocol (`SERVICE/PROTOCOL`)

`Protocol` converts between JSON and the C++ structs in `PROTOCOL_int.h` (`Command`, `ModeParams`, `Odometry`, `Sensors`, `Status`) with no WiFi or clock dependency. It reuses one `JsonDocument`, so it isn't thread-safe.

- `decode()` validates the header (`type`, `seq`, `t`) and the fields for that type, and fills a `Command`. Absent optional numbers become `NAN`; names (`"TWIST"`, `"NORTH"`, ...) are looked up in tables whose order must match the enums. On failure `error()` names the reason (`bad_json`, `bad_mode`, ...).
- `encodeOdometry/Status/Sensors/Ack/Pong()` write one message into a buffer and return its length, or 0 if it doesn't fit in `PROTOCOL_TX_BUF_SIZE` (320 bytes). `NAN` range and unread IR are written as `null`.
- `PROTOCOL_pc*Name()` turn enums into their wire names, also used for logging.

== Comms (`SERVICE/COMM`)

`Comms` combines `UdpLink` and `Protocol` and adds the link rules:
- `poll()` decodes each waiting packet, drops invalid ones and old `seq`s (`rx_bad`), accepts the rest (`rx_ok`, peer, watchdog), answers `ping` with `pong` itself, swallows `heartbeat`, and returns the first other command to the caller.
- An old `seq` is only rejected while the link is alive, so a restarted laptop script can reconnect after the watchdog.
- `linkAlive()` is true while the last valid packet is under 300 ms old.
- `send*()` stamp each outgoing message with the next robot `seq` and `millis()`; `sendStatus()` fills in `rx_ok` / `rx_bad`.

The full message format and rules are in `docs/protocol.typ`.

= HAL drivers

== MPU6050 (`HAL/MPU6050`)

#table(
  columns: (auto, 1fr),
  table.header[Function][Description],
  [`MPU6050_vInit(port)`], [Writes 0 to `PWR_MGMT_1` (0x6B): clears SLEEP, internal 8 MHz oscillator. Returns `I2C_Status_t`.],
  [`MPU6050_vReadData(port, *data)`], [Burst-reads 14 bytes from `ACCEL_XOUT_H` (0x3B): accel X/Y/Z, temperature (skipped), gyro X/Y/Z. Scales accel by 16384 LSB/g (#sym.plus.minus 2 g) and gyro by 131 LSB/(°/s) (#sym.plus.minus 250 °/s) into `MPU6050_Data_t`.],
)

The outputs are in *g* and *degrees per second*, not SI units, so the control loop must convert the gyro rate to rad/s. The functions return a status despite their `v` prefix. The sensor sits at I2C address `0x68` (AD0 low).

== Wheel encoders (`HAL/PCNT`)

Uses the ESP32 pulse counter peripheral for 4#sym.times quadrature decoding, so counting costs no CPU time.

#table(
  columns: (auto, 1fr),
  table.header[Function][Description],
  [`PCNT_vInitEncoder(unit, pinA, pinB)`], [Configures both channels of a unit (A as pulse/B as control, and B as pulse/A as control) so every edge of both phases counts. Limits #sym.plus.minus 30000, glitch filter of 100 APB cycles (1.25 µs at 80 MHz). Clears and starts the counter.],
  [`PCNT_s16GetCount(unit)`], [Current 16-bit count.],
  [`PCNT_vClearCount(unit)`], [Resets the count to 0.],
)

The hardware counter is only 16 bits and wraps at the limits, so the control loop must read it often and accumulate the deltas (planned in `vOdometryStep()`).

= MCAL drivers

== GPIO (`MCAL/GPIO`)

#table(
  columns: (auto, 1fr),
  table.header[Function][Description],
  [`GPIO_vInitPin(pin, mode)`], [`GPIO_MODE_OUTPUT`, `GPIO_MODE_INPUT`, `GPIO_MODE_INPUT_PULLUP` or `GPIO_MODE_INPUT_PULLDOWN`; interrupts off. An unknown mode does nothing.],
  [`GPIO_vSetPinValue(pin, value)`], [Writes the set/clear registers directly (`out_w1ts`/`out_w1tc`, or `out1_*` for pins 32+), so it's fast enough for the control loop.],
  [`GPIO_vTogglePin(pin)`], [Reads the pin and writes the opposite level.],
  [`GPIO_u8GetPinValue(pin)`], [Reads the input register; returns 0 or 1.],
)

== I2C (`MCAL/I2C`) <mcal-i2c>

#table(
  columns: (auto, 1fr),
  table.header[Function][Description],
  [`I2C_xInitMaster(port, sda, scl, hz)`], [Master mode with internal pull-ups, installs the ESP-IDF driver.],
  [`I2C_xWriteRegister(port, addr, reg, data)`], [Writes one byte to one register.],
  [`I2C_xReadRegister(port, addr, reg, *data)`], [Reads one register.],
  [`I2C_xReadBytes(port, addr, reg, *buf, len)`], [Burst read starting at `reg`.],
)

All calls time out after `I2C_MASTER_TIMEOUT_MS` (1 s) and return `I2C_OK` or `I2C_ERR_FAIL`.

#note[The 1 s timeout blocks the caller. Once the control loop reads the IMU, a missing sensor would stall the 1 kHz loop, so the timeout will need to come down to about 1 ms.]

== PWM (`MCAL/PWM`)

Wraps the ESP32 LEDC peripheral.

#table(
  columns: (auto, 1fr),
  table.header[Function][Description],
  [`PWM_vInitTimer(mode, timer, hz, resolution)`], [Configures an LEDC timer.],
  [`PWM_vInitChannel(mode, channel, timer, gpio)`], [Attaches a channel to a timer and a pin, duty 0.],
  [`PWM_vSetDuty(mode, channel, duty)`], [Sets and applies a raw duty value.],
  [`PWM_vSetServoAngle(mode, channel, angle)`], [Clamps to 0–180°, maps to a 500–2500 µs pulse and converts to duty ticks for a 20 ms period: $"duty" = "pulse" dot (2^13 - 1) \/ 20000$. Assumes the channel's timer is set to 50 Hz, 13-bit.],
)

Intended settings (`PWM_cfg.h`): motors at 20 kHz, 10-bit (above hearing range, so no hum); servos at 50 Hz, 13-bit.

= Hardware pin map

#table(
  columns: (auto, auto, 1fr),
  table.header[GPIO][Function][Defined in],
  [21], [I2C SDA (MPU6050)], [`I2C_MASTER_SDA_PIN`, `MCAL/I2C/I2C_cfg.h`],
  [22], [I2C SCL (MPU6050)], [`I2C_MASTER_SCL_PIN`, `MCAL/I2C/I2C_cfg.h`],
  [22], [Left motor DIR1 #text(fill: rgb("#c01c28"))[(conflict, see @issues)]], [`MOTOR_LEFT_DIR1_PIN`, `MCAL/GPIO/GPIO_cfg.h`],
  [23], [Left motor DIR2], [`MOTOR_LEFT_DIR2_PIN`],
  [18], [Right motor DIR1], [`MOTOR_RIGHT_DIR1_PIN`],
  [19], [Right motor DIR2], [`MOTOR_RIGHT_DIR2_PIN`],
  [26], [Ultrasonic TRIG], [`ULTRASONIC_TRIG_PIN`, `src/main2.cpp`],
  [27], [Ultrasonic ECHO], [`ULTRASONIC_ECHO_PIN`, `src/main2.cpp`],
  [34, 35], [Left encoder A, B], [`ENCODER_LEFT_*_PIN`, `HAL/PCNT/PCNT_cfg.h`],
  [36, 39], [Right encoder A, B], [`ENCODER_RIGHT_*_PIN`, `HAL/PCNT/PCNT_cfg.h`],
  [TBD], [Motor PWM (2)], [not defined yet],
  [TBD], [IR line sensors (2)], [commented out in `CONTROL_TASK_cfg.h`],
  [TBD], [Gripper servos (2)], [not defined yet],
)

GPIO 34–39 are input-only and have no internal pull-ups, so the encoders need external pull-ups (most encoder boards have them). If the ultrasonic sensor runs at 5 V (e.g. HC-SR04), its ECHO output needs a voltage divider before GPIO 27.

= Configuration reference

All tunable values live in the `_cfg.h` files. Values marked *placeholder* must be measured on the real robot.

== `APP/COMM_TASK/COMM_TASK_cfg.h`

#table(
  columns: (auto, auto, 1fr),
  table.header[Macro][Value][Meaning],
  [`COMMTASK_CORE`], [0], [Core shared with WiFi.],
  [`COMMTASK_PRIORITY`], [5], [],
  [`COMMTASK_STACK_BYTES`], [6144], [],
  [`COMMTASK_PERIOD_MS`], [1], [Packet polling period.],
  [`COMMTASK_TWIST_STALE_MS`], [300], [Twist older than this #sym.arrow $v = omega = 0$.],
  [`COMMTASK_TWIST_LOG_MS`], [500], [Twist log period.],
  [`COMMTASK_GRIPPER_BOOT_LIFT` / `_GRIP`], [`Up` / `Open`], [Gripper state assumed at boot.],
)

== `APP/CONTROL_TASK/CONTROL_TASK_cfg.h`

#table(
  columns: (auto, auto, 1fr),
  table.header[Macro][Value][Meaning],
  [`CONTROLTASK_CORE`], [1], [Reserved for the control loop.],
  [`CONTROLTASK_PRIORITY`], [10], [Highest on core 1.],
  [`CONTROLTASK_STACK_BYTES`], [4096], [],
  [`CONTROLTASK_PERIOD_MS`], [1], [1 kHz loop.],
  [`CONTROLTASK_ARENA_SIZE_M`], [3.0], [Arena side length (*placeholder*).],
  [`CONTROLTASK_IR_FORWARD_OFFSET_M`], [0.10], [IR sensor line ahead of the robot's center (*placeholder*).],
  [`CONTROLTASK_BOOT_X_M`, `_Y_M`], [0.15, 0.15], [Rough start pose.],
  [`CONTROLTASK_BOOT_FACING`], [`Side::North`], [Start heading.],
  [`CONTROLTASK_WHEEL_RADIUS_M`], [0.033], [*Placeholder.*],
  [`CONTROLTASK_TRACK_WIDTH_M`], [0.15], [Distance between wheel centers (*placeholder*).],
  [`CONTROLTASK_IR_LEFT_PIN`, `_RIGHT_PIN`, `_ON_LINE_LEVEL`], [commented out], [IR pins and active level. Left undefined on purpose: a floating pin would trip the edge guard at random.],
)

== `SERVICE/COMM/COMM_cfg.h`

#table(
  columns: (auto, auto, 1fr),
  table.header[Macro][Value][Meaning],
  [`COMM_WATCHDOG_MS`], [300], [No valid packet for this long #sym.arrow link dead.],
  [`COMM_ODOMETRY_PERIOD_MS`], [50], [20 Hz.],
  [`COMM_STATUS_PERIOD_MS`], [200], [5 Hz, plus on change.],
  [`COMM_SENSORS_PERIOD_MS`], [100], [10 Hz.],
)

== `SERVICE/PROTOCOL/PROTOCOL_cfg.h`

#table(
  columns: (auto, auto, 1fr),
  table.header[Macro][Value][Meaning],
  [`PROTOCOL_VERSION`], [1], [Message schema version.],
  [`PROTOCOL_TX_BUF_SIZE`], [320], [Largest encoded outgoing message (bytes).],
)

== `SERVICE/UDP/UDP_cfg.h` <cfg-udp>

#table(
  columns: (auto, auto, 1fr),
  table.header[Macro][Value][Meaning],
  [`UDP_WIFI_SSID`, `UDP_WIFI_PASS`], [(router)], [2.4 GHz network credentials.],
  [`UDP_USE_STATIC_IP`], [1], [0 = use DHCP.],
  [`UDP_STATIC_IP`], [192.168.69.50], [Must be in the router's subnet and outside its DHCP pool.],
  [`UDP_GATEWAY`, `UDP_SUBNET`], [192.168.69.1, 255.255.255.0], [],
  [`UDP_CONNECT_TIMEOUT_MS`], [10000], [Boot wait for WiFi.],
  [`UDP_LOCAL_PORT`], [5005], [],
  [`UDP_MAX_PAYLOAD`], [1472], [Largest datagram (one 1500-byte frame).],
)

== HAL and MCAL

#table(
  columns: (auto, auto, 1fr),
  table.header[Macro][Value][File],
  [`MPU6050_I2C_ADDR`], [0x68], [`HAL/MPU6050/MPU_cfg.h`],
  [`MPU6050_ACCEL_SENSITIVITY`], [16384.0], [LSB/g at #sym.plus.minus 2 g],
  [`MPU6050_GYRO_SENSITIVITY`], [131.0], [LSB/(°/s) at #sym.plus.minus 250 °/s],
  [`PCNT_H_LIM_VAL`, `PCNT_L_LIM_VAL`], [#sym.plus.minus 30000], [`HAL/PCNT/PCNT_cfg.h`],
  [`PCNT_GLITCH_FILTER_VAL`], [100], [APB cycles],
  [`I2C_MASTER_FREQ_HZ`], [400000], [`MCAL/I2C/I2C_cfg.h`],
  [`I2C_MASTER_TIMEOUT_MS`], [1000], [`MCAL/I2C/I2C_cfg.h`],
  [`LEDC_MOTOR_PWM_FREQ_HZ`, `_RES`], [20000, 10-bit], [`MCAL/PWM/PWM_cfg.h`],
  [`LEDC_SERVO_PWM_FREQ_HZ`, `_RES`], [50, 13-bit], [`MCAL/PWM/PWM_cfg.h`],
  [`LEDC_SERVO_MIN/MAX_PULSE_US`], [500, 2500], [`MCAL/PWM/PWM_cfg.h`],
)

= Laptop tools

All scripts are in `tools/`, need Python 3.10+, and use only the standard library. Run them from that folder with the laptop on the robot's router.

#table(
  columns: (auto, 1fr),
  table.header[Script][What it does],
  [`robot_link.py`], [The `RobotLink` client library: background heartbeat/twist sender at 50 Hz, receiver thread, acked commands with retries, latest telemetry. API in `docs/protocol.typ`.],
  [`ping_test.py`], [20 pings; prints RTT min/median/max and loss.],
  [`heartbeat_test.py`], [APPROACH refused; TWIST streaming with measured odometry/status rates; pauses the sender to time the watchdog (expect about 300 ms); resumes.],
  [`mock_test.py`], [Checks local argument validation, gripper in STOP and TWIST, `square_ref` refused outside SQUARE, SQUARE #sym.arrow WAITING #sym.arrow `square_ref` SOUTH/WEST with the resulting pose, all MOVE/TURN stub failure reasons, and the 10 Hz sensors stream. Prints a pass/fail summary.],
)

= Known issues <issues>

#warn[*GPIO 22 is assigned twice:* `I2C_MASTER_SCL_PIN` and `MOTOR_LEFT_DIR1_PIN` are both 22. Driving the left motor direction pin would break the IMU bus. One of them must move before the motor driver is wired in.]

+ *Protocol doc and firmware disagree on odometry.* `docs/protocol.md` says odometry is all zeros until the encoders are integrated, but the firmware reports the boot pose $(0.15, 0.15, pi/2)$ and applies `square_ref` corrections. `docs/protocol.typ` describes the actual behavior.
+ *`mock_test.py` arena size mismatch.* It mirrors the config with `ARENA_SIZE_M = 2.0`, but `CONTROLTASK_ARENA_SIZE_M` is 3.0. Harmless today because the test only squares on SOUTH and WEST, which don't use the arena size.
+ *Wrong-type optional parameters are silently dropped.* `"distance": "0.5"` (a string) decodes as absent, so MOVE fails with `missing_distance` instead of the packet being rejected.
+ *Strict `WHO_AM_I` check.* Boot only wakes the MPU6050 if `WHO_AM_I` reads `0x68`. Many GY-521 clones report another value (e.g. 0x70, 0x72, 0x98) and would be left asleep.
+ *Blocking I2C timeout* of 1 s (see the note in @mcal-i2c), which matters once the IMU is read from the 1 kHz loop.
+ *Boot can take 10 s* without a router, delaying the control task start.
+ *WiFi credentials are committed* in `UDP_cfg.h`.
+ *Oversized datagrams* (over 1472 bytes) end that tick's receive loop early; remaining packets are handled 1 ms later.
+ `docs/protocol.md` and `src/SERVICE/PROTOCOL/PROTOCOL_cfg.h` still point to the Markdown protocol document.
