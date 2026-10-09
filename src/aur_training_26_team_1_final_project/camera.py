import cv2
from PySide6.QtCore import QThread, Signal, Qt
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

class VideoCaptureThread(QThread):
    # Signal to transmit the processed QImage to the UI thread
    frame_received = Signal(QImage)
    connection_failed = Signal(str)

    def __init__(self, stream_url: str):
        super().__init__()
        self._stream_url = stream_url
        self._is_running = True

    def run(self):
        # Open video capture stream (supports HTTP, RTSP, and local webcam indices)
        cap = cv2.VideoCapture(self._stream_url)

        if not cap.isOpened():
            self.connection_failed.emit(f"Could not connect to:\n{self._stream_url}")
            return

        while self._is_running:
            ret, frame = cap.read()
            if not ret:
                continue

            # OpenCV outputs BGR; convert to RGB for Qt
            rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            h, w, ch = rgb_frame.shape
            bytes_per_line = ch * w

            # Construct QImage from raw numpy data
            q_image = QImage(
                rgb_frame.data,
                w,
                h,
                bytes_per_line,
                QImage.Format.Format_RGB888
            )

            # Emit a copy so memory remains valid across thread boundaries
            self.frame_received.emit(q_image.copy())

        cap.release()

    def stop(self):
        self._is_running = False
        self.wait()


class MyCamera(QWidget):
    def __init__(self):
        super().__init__()

        self._cap_thread = None

        # Controls
        self._ctrl_layout = QHBoxLayout()
        self._url_input = QLineEdit()
        self._url_input.setPlaceholderText("e.g., http://192.168.1.3:8080/video")
        self._btn_connect = QPushButton("Connect")
        self._btn_connect.clicked.connect(self.toggle_stream)

        self._ctrl_layout.addWidget(QLabel("Stream URL:"))
        self._ctrl_layout.addWidget(self._url_input)
        self._ctrl_layout.addWidget(self._btn_connect)

        # Video Display Label
        self._video_label = QLabel("Camera feed will appear here")
        self._video_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._video_label.setStyleSheet("background-color: #1e1e1e; color: #888;")
        
        self._layout = QVBoxLayout(self)
        self._layout.addLayout(self._ctrl_layout)
        self._layout.addWidget(self._video_label)

    def toggle_stream(self):
        if self._cap_thread and self._cap_thread.isRunning():
            self.stop_stream()
        else:
            url = self._url_input.text().strip()
            if not url:
                QMessageBox.warning(self, "Error", "Please provide a valid stream URL.")
                return

            self._cap_thread = VideoCaptureThread(url)
            self._cap_thread.frame_received.connect(self.update_frame)
            self._cap_thread.connection_failed.connect(self.handle_error)
            self._cap_thread.start()

            self._btn_connect.setText("Disconnect")

    def update_frame(self, image: QImage):
        # Scale the pixmap smoothly to fit the QLabel bounds
        pixmap = QPixmap.fromImage(image)
        scaled_pixmap = pixmap.scaled(
            self._video_label.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation
        )
        self._video_label.setPixmap(scaled_pixmap)

    def handle_error(self, message: str):
        QMessageBox.critical(self, "Stream Error", message)
        self.stop_stream()

    def stop_stream(self):
        if self._cap_thread:
            self._cap_thread.stop()
            self._cap_thread = None
        self._btn_connect.setText("Connect")
        self._video_label.clear()
        self._video_label.setText("Camera feed stopped")

    def closeEvent(self, event):
        # Ensure thread cleanup when exiting the app
        self.stop_stream()
        event.accept()