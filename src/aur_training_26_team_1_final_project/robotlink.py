"""Laptop-side client for the robot. Message format: docs/protocol.md.

The robot runs a WiFi access point ("AUR-Team1-Robot") and listens for UDP on
192.168.4.1:5005. RobotLink handles the protocol details for you:
  - keeps the link alive: a background thread sends 50 Hz heartbeats, or
    your latest twist while the robot is in TWIST mode;
  - fills in `seq` and `t` on every message;
  - retries mode commands until the robot acknowledges them;
  - keeps the newest odometry and status for you to read at any time.

Example:
    from robot_link import RobotLink

    with RobotLink() as robot:
        ok, err = robot.set_mode("TWIST")
        robot.set_twist(0.2, 0.0)          # m/s, rad/s; streamed until changed
        time.sleep(2)
        print(robot.latest_odometry())
        robot.set_mode("STOP")

Safety: if the robot hears nothing valid for 300 ms (e.g. this script
crashes), it stops and reports FAILED/watchdog. After that it is in STOP, so
call set_mode() again to resume.

Requires Python 3.10+.
"""
import json
import socket
import threading
import time

DEFAULT_HOST = "192.168.4.1"
DEFAULT_PORT = 5005
MAX_PAYLOAD = 1472

MODES = ("STOP", "WHEEL_VEL", "TWIST", "MOVE", "TURN", "SQUARE", "APPROACH")


class RobotLink:
    """Connection to one robot. Use as a context manager, or call start()/close()."""

    def __init__(self, host: str = DEFAULT_HOST, port: int = DEFAULT_PORT, rate_hz: float = 50.0):
        """
        Args:
            host: Robot IP address.
            port: Robot UDP port.
            rate_hz: How often the background thread sends heartbeat/twist.
        """
        self.addr = (host, port)
        self.period = 1.0 / rate_hz
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.settimeout(0.1)
        self._t0 = time.monotonic()

        self._lock = threading.Lock()
        self._tx_lock = threading.Lock()
        # The robot drops seqs at or below the last one it accepted. Starting
        # from wall-clock ms means a restarted script is always ahead of the
        # previous one (which sent far fewer than 1000 packets/s).
        self._seq = int(time.time() * 1000) & 0x7FFFFFFF
        self._twist = (0.0, 0.0)
        self._streaming_twist = False
        self._sender_paused = False
        self._odometry = None
        self._status = None

        self._replies = []                      # unclaimed ack/pong messages
        self._replies_cv = threading.Condition()

        self._stop = threading.Event()
        self._threads = []

    # --- lifetime ------------------------------------------------------------

    def start(self) -> None:
        """Start the background sender and receiver threads."""
        if self._threads:
            return
        self._threads = [
            threading.Thread(target=self._sender_loop, name="robot-tx", daemon=True),
            threading.Thread(target=self._receiver_loop, name="robot-rx", daemon=True),
        ]
        for th in self._threads:
            th.start()

    def close(self) -> None:
        """Stop the threads and close the socket. The robot's watchdog then stops it."""
        self._stop.set()
        for th in self._threads:
            th.join()
        self._threads = []
        self._sock.close()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *exc):
        self.close()

    # --- commands ------------------------------------------------------------

    def set_mode(self, mode: str, retries: int = 3, timeout: float = 0.3, **params) -> tuple[bool, str]:
        """Switch the robot's behavior and wait for it to acknowledge.

        Args:
            mode: One of MODES.
            retries: How many times to send before giving up.
            timeout: Seconds to wait for each acknowledgement.
            **params: Mode parameters, e.g. distance=0.5 for MOVE (see docs/protocol.md).
        Returns:
            (ok, err): ok is False if the robot refused (err says why, e.g.
            "not_implemented") or never answered (err == "timeout").
        """
        if mode not in MODES:
            raise ValueError(f"unknown mode {mode!r}; expected one of {MODES}")
        with self._lock:
            self._streaming_twist = False  # don't send twists into a different mode
        for _ in range(retries):
            seq = self._send({"type": "mode", "mode": mode, **params})
            reply = self._wait_reply(lambda m: m.get("type") == "ack" and m.get("ack_seq") == seq, timeout)
            if reply is not None:
                ok = bool(reply.get("ok"))
                if ok and mode == "TWIST":
                    with self._lock:
                        self._twist = (0.0, 0.0)
                        self._streaming_twist = True
                return ok, reply.get("err", "")
        return False, "timeout"

    def set_twist(self, v: float, omega: float) -> None:
        """Set the velocity streamed while in TWIST mode.

        Args:
            v: Forward speed (m/s).
            omega: Turn rate (rad/s), positive = counter-clockwise.
        """
        with self._lock:
            self._twist = (float(v), float(omega))

    def ping(self, timeout: float = 1.0) -> float | None:
        """Measure round-trip time.

        Returns:
            RTT in ms, or None if no reply arrived in time.
        """
        t0 = time.perf_counter()
        sent_t = self._now_ms()
        self._send({"type": "ping"}, t=sent_t)
        reply = self._wait_reply(lambda m: m.get("type") == "pong" and m.get("ping_t") == sent_t, timeout)
        if reply is None:
            return None
        return (time.perf_counter() - t0) * 1000

    # --- robot state ---------------------------------------------------------

    def latest_odometry(self) -> dict | None:
        """Newest odometry message (x, y, theta, v, omega, ...), or None if none yet."""
        with self._lock:
            return dict(self._odometry) if self._odometry else None

    def latest_status(self) -> dict | None:
        """Newest status message (mode, phase, result, reason, ...), or None if none yet."""
        with self._lock:
            return dict(self._status) if self._status else None

    # --- internals -----------------------------------------------------------

    def _now_ms(self) -> int:
        return int((time.monotonic() - self._t0) * 1000)

    def _send(self, msg: dict, t: int | None = None) -> int:
        """Add seq and t, send, and return the seq used."""
        # One lock around numbering and sending, so packets from the sender
        # thread and the caller leave in seq order (the robot drops old seqs).
        with self._tx_lock:
            self._seq += 1
            seq = self._seq
            msg = {**msg, "seq": seq, "t": self._now_ms() if t is None else t}
            data = json.dumps(msg, separators=(",", ":")).encode()
            if len(data) > MAX_PAYLOAD:
                raise ValueError(f"message is {len(data)} bytes, max is {MAX_PAYLOAD}")
            self._sock.sendto(data, self.addr)
        return seq

    def _pause_sender(self, paused: bool) -> None:
        """Testing only: stop/resume the background stream to trip the robot's watchdog."""
        with self._lock:
            self._sender_paused = paused

    def _wait_reply(self, match, timeout: float) -> dict | None:
        deadline = time.monotonic() + timeout
        with self._replies_cv:
            while True:
                for i, msg in enumerate(self._replies):
                    if match(msg):
                        return self._replies.pop(i)
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    return None
                self._replies_cv.wait(remaining)

    def _sender_loop(self) -> None:
        next_send = time.monotonic()
        while not self._stop.is_set():
            with self._lock:
                paused = self._sender_paused
                streaming = self._streaming_twist
                v, omega = self._twist
            if not paused:
                try:
                    if streaming:
                        self._send({"type": "twist", "v": v, "omega": omega})
                    else:
                        self._send({"type": "heartbeat"})
                except OSError:
                    pass  # e.g. WiFi dropped; keep trying
            next_send += self.period
            self._stop.wait(max(0.0, next_send - time.monotonic()))

    def _receiver_loop(self) -> None:
        while not self._stop.is_set():
            try:
                data, src = self._sock.recvfrom(2048)
            except (socket.timeout, OSError):
                continue
            if src[0] != self.addr[0]:
                continue
            try:
                msg = json.loads(data)
            except ValueError:
                continue
            if not isinstance(msg, dict):
                continue
            kind = msg.get("type")
            if kind == "odometry":
                with self._lock:
                    self._odometry = msg
            elif kind == "status":
                with self._lock:
                    self._status = msg
            elif kind in ("ack", "pong"):
                with self._replies_cv:
                    self._replies.append(msg)
                    del self._replies[:-32]  # drop unclaimed replies beyond 32
                    self._replies_cv.notify_all()