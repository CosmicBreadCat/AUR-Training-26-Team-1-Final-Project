import time
import cv2
from PySide6.QtCore import QThread, Signal, Qt, QTimer
from PySide6.QtGui import QImage, QPixmap
from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QLineEdit, QPushButton, QMessageBox
from aur_training_26_team_1_final_project.cv.box_detection import MyDetector

class VideoCaptureThread(QThread):
    frame_received = Signal(QImage)
    connection_failed = Signal(str)
    running_toggled = Signal(bool)

    def __init__(self, stream_url: str):
        super().__init__()
        self._stream_url = stream_url
        self._is_running = False
        self.fps = 0.0
        self.frames = 0
        self._last_update_fps_t = 0
        self._last_received_frame_t = 0

    def run(self):
        cap = cv2.VideoCapture(self._stream_url)

        if not cap.isOpened():
            self.connection_failed.emit(f"Could not connect to:\n{self._stream_url}")
            return

        self._last_update_fps_t = time.monotonic()
        self._last_received_frame_t = time.monotonic()

        while self.is_running:
            ret, frame = cap.read()
            if not ret:
                if time.monotonic() - self._last_received_frame_t >= 1:
                    self.connection_failed.emit(f"Connection Interrupted to:\n{self._stream_url}")
                    self.is_running = False
                    return
                continue

            self._last_received_frame_t = time.monotonic()
            self.frames += 1

            if time.monotonic() - self._last_update_fps_t >= 0.2:
                self.fps = self.frames * 1.0 / (time.monotonic() - self._last_update_fps_t)
                self.frames = 0
                self._last_update_fps_t = time.monotonic()

            rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            h, w, ch = rgb_frame.shape
            bytes_per_line = ch * w

            q_image = QImage(rgb_frame.data, w, h, bytes_per_line, QImage.Format.Format_RGB888)

            self.frame_received.emit(q_image.copy())

        cap.release()

    def _stop(self):
        self.fps = 0.0
        self.wait()

    @property
    def is_running(self):
        return self._is_running

    @is_running.setter
    def is_running(self, new: bool):
        self._is_running = new
        self.running_toggled.emit(self._is_running)
        if new == False:
            self._stop()


class MyCamera(QWidget):
    connection_updated = Signal(int)
    fps_updated = Signal(float)
    data_updated = Signal(list)

    def __init__(self):
        super().__init__()

        self._cap_thread = None
        self.detector = MyDetector()

        self._ctrl_layout = QHBoxLayout()
        self._url_input = QLineEdit()
        self._url_input.setPlaceholderText("e.g., http://192.168.1.3:8080/video")
        self._btn_connect = QPushButton("Connect")
        self._btn_connect.clicked.connect(self._toggle_stream)

        self._ctrl_layout.addWidget(QLabel("Stream URL:"))
        self._ctrl_layout.addWidget(self._url_input)
        self._ctrl_layout.addWidget(self._btn_connect)

        self._timer = QTimer()
        self._timer.setInterval(1000)
        self._timer.timeout.connect(self._get_data)
        self._timer.start()

        self._prev_frame = None
        self._data = None

        # Video Display Label
        self._video_label = QLabel("Camera feed will appear here")
        self._video_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._video_label.setStyleSheet("background-color: #1e1e1e; color: #888;")
        self._layout = QVBoxLayout(self)
        self._layout.addLayout(self._ctrl_layout)
        self._layout.addWidget(self._video_label)

    def _toggle_stream(self):

        if not self._cap_thread or self._cap_thread.is_running == False:
            url = self._url_input.text().strip()
            if not url:
                QMessageBox.warning(self, "Error", "Please provide a valid stream URL.")
                return

            self._cap_thread = VideoCaptureThread(url)
            self._cap_thread.connection_failed.connect(self._handle_error)
            self._cap_thread.frame_received.connect(self._update_frame)
            self._cap_thread.running_toggled.connect(self._update_running_state)
            self._cap_thread.start()
            self._cap_thread.is_running = True
        else:
            self._cap_thread.is_running = False

    def _update_running_state(self, running: bool):
        if running:
            self.connection_updated.emit(1)
            self._btn_connect.setText("Disconnect")
        else:
            self.connection_updated.emit(0)
            self._btn_connect.setText("Connect")
            self._stop_stream()

    def _update_frame(self, image: QImage):
        self._prev_frame = image
        pixmap = QPixmap.fromImage(image)
        scaled_pixmap = pixmap.scaled(
            self._video_label.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation
        )
        self._video_label.setPixmap(scaled_pixmap)
        if self._cap_thread:
            self.fps_updated.emit(self._cap_thread.fps)

    def _get_data(self):
        if self._prev_frame == None:
            return
        self._data = self.detector.get_data(self._prev_frame)

        self.data_updated.emit(self._data["qr_readings"])

    def _handle_error(self, message: str):
        QMessageBox.critical(self, "Stream Error", message)
        self.stop_stream()

    def _stop_stream(self):
        if self._cap_thread:
            self._cap_thread = None
        self._video_label.clear()
        self._video_label.setText("Camera feed stopped")
        self.fps_updated.emit(0.0)

    def _closeEvent(self, event):
        self.stop_stream()
        event.accept()