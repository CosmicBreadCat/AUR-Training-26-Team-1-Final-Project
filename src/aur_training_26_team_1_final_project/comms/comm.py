from aur_training_26_team_1_final_project.comms.robotlink import RobotLink
from PySide6.QtCore import QObject, Signal, QTimer, Slot
import math
import time

class MyCommunicationChannel(QObject):
    updated_odometry_data = Signal(dict)
    qr_box_dropped = Signal(tuple)
    connection_status = Signal(bool)
    finished = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)

        self._mode = None 
        self._is_running = False   

        # 0, 1 -> TWIST Movement
        # 4 -> TOGGLE TWIST/STOP
        # 5 -> TOGGLE Gripper Closed/Open
        # 6 -> TOGGLE Gripper Elevate/Dive
        # 7 -> Save Location
        self._data = []            

        # RobotLink instance
        self.robot = None

        # Statistics & timing
        self._odom_count = 0
        self._status_count = 0
        self._last_odom = None
        self._last_status_t = None
        self._next_print = 0.0
        self._next_toggle = 0.0
        self._end_time = 0.0
        self._start_time = 0.0

        # Communication loop timer
        self._comm_timer = QTimer(self)
        self._comm_timer.setInterval(20)  # 20 ms interval (~50 Hz)
        self._comm_timer.timeout.connect(self._communication_step)

    @Slot(list)
    def get_input(self, data: list):
        self._data = list(data)

    def _get_vel_resultant(self, v_x, v_y):
        vel = round(math.sqrt(v_x ** 2 + v_y ** 2), 3)
        omega = round(math.atan2(v_y, v_x), 3)
        return vel, omega

    def fmt_status(self, s):
        if s is None:
            return "no status yet"
        reason = f"/{s['reason']}" if s.get("reason") else ""
        return (f"mode={s['mode']} phase={s['phase']} result={s['result']}{reason} "
                f"cmd_seq={s['cmd_seq']} watchdog={s['watchdog']} rx_ok={s['rx_ok']} rx_bad={s['rx_bad']}")

    def start_communication(self):
        """Initializes the robot connection and starts the periodic timer."""
        if self._is_running:
            return

        try:
            # Enter RobotLink context
            self.robot = RobotLink()
            self.robot.__enter__()

            ok, err = self.robot.set_mode("TWIST")
            if not ok:
                print(f"[TXRX] Failed to set TWIST mode: {err}")
                self.connection_status.emit(False)
                self._cleanup_robot()
                return

            self._mode = "TWIST"
            self.connection_status.emit(True)
            self._is_running = True

            # Reset stats & timers
            self._odom_count = 0
            self._status_count = 0
            self._last_odom = None
            self._last_status_t = None
            self._start_time = time.monotonic()
            self._next_print = self._start_time
            self._end_time = self._start_time + 15 * 60.0  # 15 minutes limit

            # Start non-blocking periodic timer
            self._comm_timer.start()
            print("[TXRX] Communication timer started.")

        except Exception as e:
            print(f"[TXRX] Error starting communication: {e}")
            self._cleanup_robot()

    @Slot()
    def _communication_step(self):
        """Called every 20ms by QTimer instead of a while True + sleep loop."""
        now = time.monotonic()

        # Check if 15 minutes duration expired or stopped
        if now >= self._end_time or not self._is_running:
            self.stop_communication()
            return

        if len(self._data) > 0:
            if self._data[4] == 1 and now >= self._next_toggle:
                self._next_toggle = now + 0.5
                if self._mode == "TWIST":
                    print("In STOP MODE")
                    self._mode = "STOP"
                    self.robot.set_mode("STOP")
                elif self._mode == "STOP":
                    print("In TWIST MODE")
                    self._mode = "TWIST"
                    self.robot.set_mode("TWIST")

            if self._data[5] == 1 and now >= self._next_toggle:
                self._next_toggle = now + 0.5
                # Close/Open Gripper
                print("Gripper Open/Close Toggle [NOT IMPLEMENTED FROM COMM.PY]")
            
            if self._data[6] == 1 and now >= self._next_toggle:
                self._next_toggle = now + 0.5
                # Elevate/Dive Gripper
                print("Gripper Elevate/Dive Toggle [NOT IMPLEMENTED FROM COMM.PY]")

            if self._data[7] == 1 and now >= self._next_toggle:
                self._next_toggle = now + 0.5
                # Save Location
                if self._last_odom:
                    self.qr_box_dropped.emit((self._last_odom["x"], self._last_odom["y"]))

            if self._mode == "TWIST":
                vel, omega = self._get_vel_resultant(self._data[0], self._data[1])
                self.robot.set_twist(vel, omega)

        # 2. Poll odometry
        o = self.robot.latest_odometry()
        if o:
            self._odom_count += 1
            self._last_odom = o
            self.updated_odometry_data.emit(o)

        # 3. Poll status
        s = self.robot.latest_status()
        if s and s.get("t") != self._last_status_t:
            self._status_count += 1
            self._last_status_t = s["t"]

        # 4. Periodic logging (every 0.5s)
        if now >= self._next_print:
            print(f"{self._next_print:.2f}    {self.fmt_status(s)}")
            self._next_print += 0.5

    def stop_communication(self):
        """Stops the timer, sets STOP mode, and tears down the robot connection."""
        if not self._is_running:
            return

        self._is_running = False
        self._comm_timer.stop()

        elapsed = max(time.monotonic() - self._start_time, 0.001)
        print("[TXRX] Stop event received. Exiting.")
        print(f"    odometry {self._odom_count / elapsed:.1f} Hz (expect ~20), "
              f"status {self._status_count / elapsed:.1f} Hz (expect ~5)")

        if self.robot:
            try:
                self.robot.set_mode("STOP")
            except Exception as e:
                print(f"[TXRX] Error setting STOP mode: {e}")
            self._cleanup_robot()

        self.connection_status.emit(False)
        self.finished.emit()

    def _cleanup_robot(self):
        if self.robot:
            try:
                self.robot.__exit__(None, None, None)
            except Exception:
                pass
            self.robot = None