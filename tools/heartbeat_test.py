#!/usr/bin/env python3
"""Exercise modes, twist streaming, telemetry rates and the robot's watchdog.

Steps:
  1. MOVE should be refused with "not_implemented" (not built yet).
  2. TWIST is accepted; stream v=0.2 m/s for 3 s while printing status.
  3. Pause the background stream (simulated laptop crash) and time how long
     the robot takes to report watchdog + STOP. Expect ~300 ms.
  4. Resume and return to STOP.
"""
import time

from robot_link import RobotLink

STREAM_S = 3.0


def fmt_status(s):
    if s is None:
        return "no status yet"
    reason = f"/{s['reason']}" if s.get("reason") else ""
    return (f"mode={s['mode']} phase={s['phase']} result={s['result']}{reason} "
            f"cmd_seq={s['cmd_seq']} watchdog={s['watchdog']} rx_ok={s['rx_ok']} rx_bad={s['rx_bad']}")


def wait_for(cond, timeout):
    """Poll cond() until true; return seconds taken, or None on timeout."""
    t0 = time.monotonic()
    while time.monotonic() - t0 < timeout:
        if cond():
            return time.monotonic() - t0
        time.sleep(0.005)
    return None


def main():
    with RobotLink() as robot:
        rtt = robot.ping()
        print(f"ping: {rtt:.1f} ms" if rtt is not None else "ping: no reply")

        print("\n[1] MOVE (expect not_implemented)")
        print("   ", robot.set_mode("MOVE", distance=0.5))

        print("\n[2] TWIST, streaming v=0.2 m/s")
        ok, err = robot.set_mode("TWIST")
        print("    set_mode:", (ok, err))
        if not ok:
            return
        robot.set_twist(0.2, 0.0)

        odom_count = status_count = 0
        last_odom_t = last_status_t = None
        next_print = time.monotonic()
        end = time.monotonic() + STREAM_S
        while time.monotonic() < end:
            o, s = robot.latest_odometry(), robot.latest_status()
            if o and o["t"] != last_odom_t:
                odom_count += 1
                last_odom_t = o["t"]
            if s and s["t"] != last_status_t:
                status_count += 1
                last_status_t = s["t"]
            if time.monotonic() >= next_print:
                print("   ", fmt_status(s))
                next_print += 0.5
            time.sleep(0.002)
        print(f"    odometry {odom_count / STREAM_S:.1f} Hz (expect ~20), "
              f"status {status_count / STREAM_S:.1f} Hz (expect ~5)")

        print("\n[3] pausing the stream (simulated laptop crash)")
        robot._pause_sender(True)
        took = wait_for(lambda: (s := robot.latest_status()) is not None
                        and s["watchdog"] and s["mode"] == "STOP", timeout=2.0)
        if took is None:
            print("    FAIL: no watchdog within 2 s")
        else:
            print(f"    watchdog after {took * 1000:.0f} ms (expect ~300)")
        print("   ", fmt_status(robot.latest_status()))

        print("\n[4] resuming")
        robot._pause_sender(False)
        wait_for(lambda: (s := robot.latest_status()) is not None and not s["watchdog"], timeout=2.0)
        print("    set_mode STOP:", robot.set_mode("STOP"))
        time.sleep(0.3)
        print("   ", fmt_status(robot.latest_status()))


if __name__ == "__main__":
    main()
