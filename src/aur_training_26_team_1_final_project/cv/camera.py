import os
import sys
import time
import cv2
from PySide6.QtCore import QThread, Signal, Qt, QTimer
from PySide6.QtGui import QImage, QPixmap
from PySide6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QMessageBox
)
from aur_training_26_team_1_final_project.cv.box_detection import MyDetector

class VideoCaptureThread(QThread):
    frame_received = Signal(QImage)
    connection_failed = Signal(str)

    def __init__(self, stream_url: str):
        super().__init__()
        self._stream_url = stream_url
        self.is_running = True
        self.fps = 0.0

    def run(self):
        cap = cv2.VideoCapture(self._stream_url)

        if not cap.isOpened():
            self.connection_failed.emit(f"Could not connect to:\n{self._stream_url}")
            return

        prev_time = time.time()
        self.fps = 0.0
        alpha = 0.9  # Smoothing factor to prevent jittery numbers

        while self.is_running:
            ret, frame = cap.read()
            if not ret:
                continue

            # Calculate instantaneous and smoothed FPS
            curr_time = time.time()
            time_diff = curr_time - prev_time
            prev_time = curr_time

            if time_diff > 0:
                current_fps = 1.0 / time_diff
                # Exponential moving average filter
                self.fps = (alpha * self.fps + (1.0 - alpha) * current_fps) if self.fps > 0 else current_fps

            # Convert BGR to RGB
            rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            h, w, ch = rgb_frame.shape
            bytes_per_line = ch * w

            q_image = QImage(
                rgb_frame.data,
                w,
                h,
                bytes_per_line,
                QImage.Format.Format_RGB888
            )

            self.frame_received.emit(q_image.copy())

        cap.release()

    def stop(self):
        self.is_running = False
        self.fps = 0.0
        self.wait()


class MyCamera(QWidget):
    connection_updated = Signal(int)
    fps_updated = Signal(float)
    data_updated = Signal(list)

    def __init__(self):
        super().__init__()

        self.cap_thread = None
        self.detector = MyDetector()

        self._ctrl_layout = QHBoxLayout()
        self._url_input = QLineEdit()
        self._url_input.setPlaceholderText("e.g., http://192.168.1.3:8080/video")
        self._btn_connect = QPushButton("Connect")
        self._btn_connect.clicked.connect(self.toggle_stream)

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

    def toggle_stream(self):
        if self.cap_thread and self.cap_thread.is_running == False:
            self.connection_updated.emit(1)
            self.stop_stream()
        else:
            self.connection_updated.emit((self.cap_thread == None or self.cap_thread.fps < 1.0))

        if self.cap_thread and self.cap_thread.isRunning():
            self.stop_stream()
        else:
            url = self._url_input.text().strip()
            if not url:
                QMessageBox.warning(self, "Error", "Please provide a valid stream URL.")
                return
                
            self.cap_thread = VideoCaptureThread(url)
            self.cap_thread.frame_received.connect(self.update_frame)
            self.cap_thread.connection_failed.connect(self.handle_error)
            self.cap_thread.start()

            self._btn_connect.setText("Disconnect")

    def update_frame(self, image: QImage):
        self._prev_frame = image
        pixmap = QPixmap.fromImage(image)
        scaled_pixmap = pixmap.scaled(
            self._video_label.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation
        )
        self._video_label.setPixmap(scaled_pixmap)
        self.fps_updated.emit(self.cap_thread.fps)

    def _get_data(self):
        if self._prev_frame == None:
            return
        self._data = self.detector.get_data(self._prev_frame)

        self.data_updated.emit(self._data["qr_readings"])
        # For debugging
        # print(self._data)

    def handle_error(self, message: str):
        QMessageBox.critical(self, "Stream Error", message)
        self.stop_stream()

    def stop_stream(self):
        if self.cap_thread:
            self.cap_thread.stop()
            self.cap_thread = None
        self._btn_connect.setText("Connect")
        self._video_label.clear()
        self._video_label.setText("Camera feed stopped")
        self.fps_updated.emit(0.0)

    def closeEvent(self, event):
        self.stop_stream()
        event.accept()