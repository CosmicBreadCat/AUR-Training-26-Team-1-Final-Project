#!/usr/bin/env python3
"""Exercise the mocked features: gripper, localization by squaring, MOVE/TURN stubs, sensors.

Steps:
  1. Local validation: bad gripper arguments raise ValueError without sending.
  2. Gripper in STOP, then in TWIST: each command is acked, status shows the
     new lift/grip, and the drive mode is unchanged.
  3. Localization: square_ref is refused unless SQUARE is WAITING; SQUARE
     (mocked) goes straight to WAITING; square_ref SOUTH then WEST corrects
     theta/y then theta/x in odometry and ends SQUARE with DONE.
  4. MOVE, TURN: acked, then the control-task stub reports FAILED
     (not_implemented, missing_distance / missing_angle, conflicting_target)
     and the robot returns to STOP.
  5. sensors: ~10 Hz, range_m / ir_left / ir_right None (no drivers yet).

TURN facing before any square_ref since boot fails with no_heading_ref; that
only reproduces right after a reboot, so it isn't checked here.
"""
import math
import time

from robot_link import RobotLink

failures = 0

# Mirror the placeholders in src/APP/CONTROL_TASK/CONTROL_TASK_cfg.h
ARENA_SIZE_M = 2.0
IR_FORWARD_OFFSET_M = 0.10


def check(cond, what):
    global failures
    print(f"    {'ok  ' if cond else 'FAIL'} {what}")
    if not cond:
        failures += 1


def wait_for(cond, timeout):
    """Poll cond() until true; return seconds taken, or None on timeout."""
    t0 = time.monotonic()
    while time.monotonic() - t0 < timeout:
        if cond():
            return time.monotonic() - t0
        time.sleep(0.005)
    return None


def gripper_round(robot, mode):
    for lift, grip in (("DOWN", None), (None, "CLOSED"), ("UP", "OPEN")):
        ok, err = robot.set_gripper(lift=lift, grip=grip)
        check(ok, f"[{mode}] set_gripper(lift={lift}, grip={grip}) acked {(ok, err)}")
        s = robot.latest_status() or {}
        want_lift = lift or s.get("lift")
        want_grip = grip or s.get("grip")
        took = wait_for(lambda: (st := robot.latest_status()) is not None
                        and st.get("lift") == want_lift and st.get("grip") == want_grip, 1.0)
        s = robot.latest_status() or {}
        check(took is not None, f"[{mode}] status lift={s.get('lift')} grip={s.get('grip')}")
        check(s.get("mode") == mode, f"[{mode}] drive mode unchanged ({s.get('mode')})")


def main():
    with RobotLink() as robot:
        rtt = robot.ping()
        print(f"ping: {rtt:.1f} ms" if rtt is not None else "ping: no reply")

        print("\n[1] local validation")
        for kwargs in ({}, {"lift": "SIDEWAYS"}, {"grip": "open"}):
            try:
                robot.set_gripper(**kwargs)
                check(False, f"set_gripper({kwargs}) should raise")
            except ValueError:
                check(True, f"set_gripper({kwargs}) raises ValueError")

        print("\n[2] gripper in STOP, then in TWIST")
        check(robot.set_mode("STOP")[0], "set_mode STOP")
        gripper_round(robot, "STOP")
        check(robot.set_mode("TWIST")[0], "set_mode TWIST")
        gripper_round(robot, "TWIST")

        print("\n[3] localization by squaring")
        try:
            robot.set_square_side("UP")
            check(False, "set_square_side('UP') should raise")
        except ValueError:
            check(True, "set_square_side('UP') raises ValueError")
        check(robot.set_square_side("NORTH") == (False, "not_waiting"),
              "square_ref in STOP -> (False, 'not_waiting')")
        d = IR_FORWARD_OFFSET_M
        for side, want in (("SOUTH", {"theta": -math.pi / 2, "y": d}),
                           ("WEST", {"theta": math.pi, "x": d, "y": d})):
            ok, err = robot.set_mode("SQUARE", max_search=0.5)
            check(ok, f"set_mode SQUARE acked {(ok, err)}")
            took = wait_for(lambda: (s := robot.latest_status()) is not None
                            and s["mode"] == "SQUARE" and s["phase"] == "WAITING", 1.0)
            check(took is not None, "SQUARE reaches phase WAITING")
            prev_seq = (robot.latest_status() or {}).get("cmd_seq")
            ok, err = robot.set_square_side(side)
            check(ok, f"set_square_side({side}) acked {(ok, err)}")
            took = wait_for(lambda: (s := robot.latest_status()) is not None
                            and s["mode"] == "STOP" and s["result"] == "DONE"
                            and s["cmd_seq"] == prev_seq, 1.0)
            check(took is not None, "SQUARE ends DONE -> STOP")
            close = lambda o: o is not None and all(abs(o[k] - v) < 1e-3 for k, v in want.items())
            wait_for(lambda: close(robot.latest_odometry()), 0.5)
            o = robot.latest_odometry() or {}
            nan = float("nan")
            check(close(o), f"odometry after {side}: x={o.get('x', nan):.3f} y={o.get('y', nan):.3f} "
                            f"theta={o.get('theta', nan):.3f} (expect {want})")

        print("\n[4] MOVE / TURN stubs")
        cases = (
            ("MOVE", {"distance": 0.5}, "not_implemented"),
            ("MOVE", {}, "missing_distance"),
            ("TURN", {"angle": 1.57}, "not_implemented"),
            ("TURN", {"facing": "NORTH"}, "not_implemented"),  # heading known after [3]
            ("TURN", {"angle": 1.57, "facing": "NORTH"}, "conflicting_target"),
            ("TURN", {}, "missing_angle"),
        )
        for mode, params, reason in cases:
            prev_seq = (robot.latest_status() or {}).get("cmd_seq")
            ok, err = robot.set_mode(mode, **params)
            check(ok, f"set_mode {mode} {params} acked {(ok, err)}")
            took = wait_for(lambda: (s := robot.latest_status()) is not None
                            and s["cmd_seq"] != prev_seq and s["mode"] == "STOP"
                            and s["result"] == "FAILED", 1.0)
            s = robot.latest_status() or {}
            check(took is not None and s.get("reason") == reason,
                  f"{mode} {params} -> mode={s.get('mode')} {s.get('result')}/{s.get('reason')} "
                  f"(expect STOP FAILED/{reason})")

        print("\n[5] sensors stream (2 s)")
        count, last_t = 0, None
        end = time.monotonic() + 2.0
        while time.monotonic() < end:
            z = robot.latest_sensors()
            if z and z["t"] != last_t:
                count += 1
                last_t = z["t"]
            time.sleep(0.002)
        z = robot.latest_sensors() or {}
        check(7 <= count / 2.0 <= 13, f"sensors {count / 2.0:.1f} Hz (expect ~10)")
        for key in ("range_m", "ir_left", "ir_right"):
            check(key in z and z[key] is None, f"{key}={z.get(key, 'missing')} (expect None)")

        robot.set_mode("STOP")

    print(f"\n{'all passed' if failures == 0 else f'{failures} FAILED'}")


if __name__ == "__main__":
    main()
