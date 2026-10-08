from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton
from PySide6.QtGui import QColor, QPalette, Qt
from PySide6.QtCore import Signal, QTimer
import cv2
import numpy as np

class MyStatus(QWidget):
    motion_started = Signal()
    motion_stopped = Signal()

    def __init__(self):
        
        super().__init__()

        print('Status Section has successfully appeared')

        #Background Color
        self.setAutoFillBackground(True)
        palette = self.palette()
        palette.setColor(QPalette.ColorRole.Window, QColor("#3e3e3e"))
        self.setPalette(palette)

        # Box Colors
        # It is okay to have lower hue greater than upper hue
        # It is intended to handle cyclic hue segments in OpenCV
        self._colors = [160, 10, 35, 85, 100, 140]

        # 0 -> Disconnected, 1 -> Connected
        self._connection_status = 0
        self._connection_status_msgs = ['Disconnected', 'Connected']

        # 0 -> None, 1 -> Semi, 2 -> Fully
        self._autonomous_mode = 0
        self._autonomous_mode_msgs = ['None', 'Semi', 'Fully']

        # 0 -> Easy, 1 -> Hard, 2 -> QR
        self._picked_boxes = 0
        self._picked_boxes_msgs = ['Easy Boxes', 'Hard Boxes', 'QR Boxes']

        self._fps = 0

        self._timer = QTimer()
        self._time = 0

        self._timer_label = QLabel('Timer: 00:00')
        self._timer_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._timer_label.setStyleSheet('font-size: 20px; font-weight: bold;')

        self._fps_label = QLabel('FPS: 0')
        self._fps_label.setStyleSheet('font-size: 14px;')

        self._connection_status_label = QLabel('Connection Status: Disconnected')
        self._connection_status_label.setStyleSheet('font-size: 14px;')

        self._autonomous_mode_label = QLabel('Autonomous Mode: None')
        self._autonomous_mode_label.setStyleSheet('font-size: 14px;')

        self._picked_boxes_label = QLabel('Boxes Picked: Easy Boxes')
        self._picked_boxes_label.setStyleSheet('font-size: 14px;')

        self._motion_status_btn = QPushButton('Start')

        self._color_widgets = []
        self._color_widgets_layout = QVBoxLayout()

        for i in range(3):
            self._color_widgets.append(QWidget())
            self._color_widgets[i].setStyleSheet(self._get_gradient_style_sheet((self._colors[i*2], 255, 255), (self._colors[i*2+1], 255, 255)))
            self._color_widgets_layout.addWidget(self._color_widgets[i])

        self._horizontal_layout = QHBoxLayout()
        self._horizontal_layout.addWidget(self._picked_boxes_label)
        self._horizontal_layout.addWidget(self._fps_label)

        self._horizontal_layout.setStretch(0, 3)
        self._horizontal_layout.setStretch(1, 1)

        self._status_msgs_layout = QVBoxLayout()
        self._status_msgs_layout.addWidget(self._connection_status_label)
        self._status_msgs_layout.addWidget(self._autonomous_mode_label)
        self._status_msgs_layout.addLayout(self._horizontal_layout)

        self._horizontal_layout_2 = QHBoxLayout()
        self._horizontal_layout_2.addLayout(self._status_msgs_layout)
        self._horizontal_layout_2.addLayout(self._color_widgets_layout)

        self._horizontal_layout_2.setStretch(0, 3)
        self._horizontal_layout_2.setStretch(1, 1)

        self._container = QVBoxLayout(self)
        self._container.addWidget(self._timer_label)
        self._container.addLayout(self._horizontal_layout_2)
        self._container.addWidget(self._motion_status_btn)

        self._container.setStretch(0, 0)
        self._container.setStretch(1, 2)
        self._container.setStretch(2, 1)

        self._motion_status_btn.clicked.connect(self._toggle_motion_status)

    @property
    def time(self):
        return self._time

    @time.setter
    def time(self, new:int):
        self._time = new
        m = str((new // 60))
        s = str((new % 60))
        while len(m) < 2:
            m = '0'+m
        while len(s) < 2:
            s = '0'+s
        self._timer_label.setText(f"Timer: {m}:{s}")

    @property
    def fps(self):
        return self._fps

    @fps.setter
    def fps(self, new:int):
        self._fps = new
        self._fps_label.setText(f"FPS: {self._fps}")

    @property
    def autonomous_mode(self):
        return self._autonomous_mode

    @autonomous_mode.setter
    def autonomous_mode(self, new:int):
        self._autonomous_mode = new
        self._autonomous_mode_label.setText(f"Autonomous Mode: {self._autonomous_mode_msgs[new]}")

    @property
    def connection_status(self):
        return self._connection_status

    @connection_status.setter
    def connection_status(self, new:int):
        self._connection_status = new
        self._connection_status_label.setText(f"Connection Status: {self._connection_status_msgs[new]}")
        
    @property
    def picked_boxes(self):
        return self._picked_boxes

    @picked_boxes.setter
    def picked_boxes(self, new:int):
        self._picked_boxes = new
        self._picked_boxes_label.setText(f"Picked Boxes: {self._picked_boxes_msgs[new]}")

    @property
    def colors(self):
        return self._colors

    @colors.setter
    def colors(self, new:list):
        for i in range(6):
            if new[i] != -1:
                self._colors[i] = new[i]

        for i in range(3):
            self._color_widgets[i].setStyleSheet(self._get_gradient_style_sheet((self._colors[i*2], 255, 255), (self._colors[i*2+1], 255, 255)))

    def _toggle_motion_status(self):

        if self._motion_status_btn.text() == 'Start':
            self.motion_started.emit()
            self._timer.setInterval(1000)
            self.time = 600
            self._timer.start()
            self._timer.timeout.connect(self._update_time)

            self._motion_status_btn.setText('Reset')
        else:
            self.motion_stopped.emit()
            self._timer.stop()
            self.time = 0
            self._motion_status_btn.setText('Start')

    def _update_time(self):
        self.time -= 1
        if self.time == 0:
            self._toggle_motion_status()

    def update_data(self, new_connection_status:int, new_autonomous_mode:int, new_picked_boxes:int, new_fps:int, new_colors:list):
        self.connection_status = new_connection_status
        self.autonomous_mode = new_autonomous_mode
        self.picked_boxes = new_picked_boxes
        self.fps = new_fps
        self.colors = new_colors

    def _hsv_to_hex(self, hsv:tuple):
        # HSV_MAX = (179, 255, 255), OpenCV is used
        hsv_color = np.uint8([[[hsv[0], hsv[1], hsv[2]]]])
        rgb_color = cv2.cvtColor(hsv_color, cv2.COLOR_HSV2RGB)

        return '#{:02x}{:02x}{:02x}'.format(rgb_color[0][0][0], rgb_color[0][0][1], rgb_color[0][0][2])

    def _get_gradient_style_sheet(self, hsv1:tuple, hsv2:tuple):

        hex1 = self._hsv_to_hex(hsv1)
        hex2 = self._hsv_to_hex(hsv2)

        return f"""background: qlineargradient(
                        x1: 0, y1: 0, 
                        x2: 1, y2: 0,
                        stop: 0 {hex1}, 
                        stop: 1 {hex2}
                    );
            """