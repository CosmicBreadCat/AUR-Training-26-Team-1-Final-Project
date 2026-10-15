#!/usr/bin/env python3
"""Ping the robot COUNT times and report round-trip time and loss."""
import statistics
import time

from robot_link import RobotLink

COUNT = 20


def main():
    with RobotLink() as robot:
        rtts = []
        for i in range(COUNT):
            rtt = robot.ping(timeout=1.0)
            print(f"ping {i + 1}: " + (f"{rtt:.1f} ms" if rtt is not None else "lost"))
            if rtt is not None:
                rtts.append(rtt)
            time.sleep(0.05)

    lost = COUNT - len(rtts)
    print(f"\nreceived {len(rtts)}/{COUNT}, lost {lost} ({100 * lost / COUNT:.0f}%)")
    if rtts:
        print(f"RTT ms: min {min(rtts):.1f}  median {statistics.median(rtts):.1f}  max {max(rtts):.1f}")


if __name__ == "__main__":
    main()
