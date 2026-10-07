from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton
from PySide6.QtGui import QColor, QPalette, Qt
from PySide6.QtCore import Signal, QTimer
import cv2
import numpy as np

class MyStatus(QWidget):

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
        self._colors = [(160, 255, 255), (10, 255, 255), (35, 255, 255), (85, 255, 255), (100, 255, 255), (140, 255, 255)]

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

        self._timer_label = QLabel('Timer: 10:00')
        self._timer_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._timer_label.setStyleSheet('font-size: 20px; font-weight: bold;')

        self._fps_label = QLabel('FPS: 0')
        self._fps_label.setStyleSheet('font-size: 14px;')

        self._connection_status_label = QLabel('Connection Status: Disconnected')
        self._connection_status_label.setStyleSheet('font-size: 14px;')

        self._autonomous_mode_label = QLabel('Autonomous Mode: None')
        self._autonomous_mode_label.setStyleSheet('font-size: 14px;')

        self._boxes_picked_label = QLabel('Boxes Picked: Easy Boxes')
        self._boxes_picked_label.setStyleSheet('font-size: 14px;')

        self._motion_start_btn = QPushButton('Start')

        self._color_widgets = []
        self._color_widgets_layout = QVBoxLayout()

        for i in range(3):
            self._color_widgets.append(QWidget())
            self._color_widgets[i].setStyleSheet(self._get_gradient_style_sheet(self._colors[i*2], self._colors[i*2+1]))
            self._color_widgets_layout.addWidget(self._color_widgets[i])

        self._horizontal_layout = QHBoxLayout()
        self._horizontal_layout.addWidget(self._boxes_picked_label)
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
        self._container.addWidget(self._motion_start_btn)

        self._container.setStretch(0, 0)
        self._container.setStretch(1, 2)
        self._container.setStretch(2, 1)

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