#!/usr/bin/env python3
"""Exercise the mocked features: gripper commands, the SQUARE stub and sensors.

Steps:
  1. Local validation: bad gripper arguments raise ValueError without sending.
  2. Gripper in STOP, then in TWIST: each command is acked, status shows the
     new lift/grip, and the drive mode is unchanged.
  3. SQUARE: acked, then the control-task stub reports FAILED/not_implemented
     and the robot returns to STOP.
  4. sensors: ~10 Hz, range_m None (no ultrasonic driver yet).
"""
import time

from robot_link import RobotLink

failures = 0


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

        print("\n[3] SQUARE stub")
        ok, err = robot.set_mode("SQUARE", max_search=1.0)
        check(ok, f"set_mode SQUARE acked {(ok, err)}")
        took = wait_for(lambda: (s := robot.latest_status()) is not None
                        and s["mode"] == "STOP" and s["result"] == "FAILED", 1.0)
        s = robot.latest_status() or {}
        check(took is not None and s.get("reason") == "not_implemented",
              f"status mode={s.get('mode')} result={s.get('result')}/{s.get('reason')}")

        print("\n[4] sensors stream (2 s)")
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
        check("range_m" in z and z["range_m"] is None, f"range_m={z.get('range_m', 'missing')} (expect None)")

        robot.set_mode("STOP")

    print(f"\n{'all passed' if failures == 0 else f'{failures} FAILED'}")


if __name__ == "__main__":
    main()
