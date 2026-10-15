from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel
from PySide6.QtGui import QColor, QPalette
from PySide6.QtCore import QTimer, Qt
from aur_training_26_team_1_final_project.gui.data import MyData
from aur_training_26_team_1_final_project.assets.compass import Compass

class MyInfo(QWidget):

    def __init__(self):
        
        super().__init__()

        #Must add compass and map later
        print('Info Section has successfully appeared')

        #Background Color
        self.setAutoFillBackground(True)
        palette = self.palette()
        palette.setColor(QPalette.ColorRole.Window, QColor("#3e3e3e"))
        self.setPalette(palette)

        self._odometry_label = QLabel('Odometry')
        self._odometry_label.setStyleSheet('font-size: 20px; font-weight: bold;')

        self._linear_speed_label = QLabel('Linear Speed: 0 m/s')
        self._angular_speed_label = QLabel('Angular Speed: 0 rad/s')
        self._coord_label = QLabel('Coordinates: (0, 0)')
        self._angle_label = QLabel('Angle: 0 degrees')
        self._qr_boxes_dropped_label = QLabel('QR Dropped Boxes: \n')
        self._qr_boxes_detected_label = QLabel('QR Detected Boxes: \n')


        self._linear_speed_label.setStyleSheet('font-size: 14px;')
        self._angular_speed_label.setStyleSheet('font-size: 14px;')
        self._coord_label.setStyleSheet('font-size: 14px;')
        self._angle_label.setStyleSheet('font-size: 14px;')
        self._qr_boxes_dropped_label.setStyleSheet('font-size: 14px;')
        self._qr_boxes_detected_label.setStyleSheet('font-size: 14px;')

        self._data = MyData(self)
        self._data.linear_speed = 0
        self._data.angular_speed = 0
        self._data.coord = (0, 0)
        self._data.angle = 0
        
        self._compass = Compass("compass.qml", self._data, self)

        self._layout_1 = QVBoxLayout()

        self._layout_1.addWidget(self._linear_speed_label)
        self._layout_1.addWidget(self._angular_speed_label)
        self._layout_1.addWidget(self._coord_label)
        self._layout_1.addWidget(self._angle_label)
        self._layout_1.addWidget(self._compass, alignment=Qt.AlignmentFlag.AlignCenter)
        self._layout_1.addWidget(self._qr_boxes_dropped_label)
        self._layout_1.addWidget(self._qr_boxes_detected_label)

        self._container = QVBoxLayout(self)

        self._container.addWidget(self._odometry_label)
        self._container.addLayout(self._layout_1)

        self._container.setStretch(0, 0)
        self._container.setStretch(1, 1)

        self._layout_1.addStretch()

        self._timer = QTimer()
        self._timer.setInterval(20)
        self._timer.timeout.connect(self._update_widgets)
        self._timer.start()

    def update_data(self, new_data:dict):
        if not new_data:
            return
        self._data.linear_speed = new_data["v"]
        self._data.angular_speed = new_data["omega"]
        self._data.angle = new_data["theta"]
        self._data.coord = (new_data["x"], new_data["y"])

    def update_qr_boxes_dropped(self, new_coord: tuple):
        self._data.qr_boxes_dropped.append(new_coord)

    def update_qr_boxes_detected(self, new: list):
        self._data.qr_boxes_detected = []
        for qr_code in new:
            self._data.qr_boxes_detected.append(qr_code["text"])

    def _update_widgets(self):
        
        self._linear_speed_label.setText(f'Linear Speed: {self._data.linear_speed}')
        self._angular_speed_label.setText(f'Angular Speed: {self._data.angular_speed}')
        self._coord_label.setText(f'Coordinates: {self._data.coord}')
        self._angle_label.setText(f'Angle: {self._data.angle}')

        text = "QR Boxes Dropped: \n"

        for coord in self._data.qr_boxes_dropped:
            text += f"{coord}\n"

        self._qr_boxes_dropped_label.setText(text)

        text = "QR Boxes Detected: \n"

        for coord in self._data.qr_boxes_detected:
            text += f"{coord}\n"
            
        self._qr_boxes_detected_label.setText(text)