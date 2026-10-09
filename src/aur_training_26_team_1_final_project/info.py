from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel
from PySide6.QtGui import QColor, QPalette
from PySide6.QtCore import Qt, Signal, QObject

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

        self._speed_label = QLabel('Speed: 0 m/s')
        self._coord_label = QLabel('Coordinates: (0, 0)')
        self._angle_label = QLabel('Angle: 0 degrees')
        self._qr_boxes_labels = []

        self._data = QObject()
        self._data.speed = 0
        self._data.coord = (0, 0)
        self._data.angle = 0

        self._layout_1 = QVBoxLayout()

        self._layout_1.addWidget(self._speed_label)
        self._layout_1.addWidget(self._coord_label)
        self._layout_1.addWidget(self._angle_label)

        self._container = QVBoxLayout(self)

        self._container.addWidget(self._odometry_label)
        self._container.addLayout(self._layout_1)

        self._container.setStretch(0, 0)
        self._container.setStretch(1, 1)

        self._layout_1.addStretch()

    def update_data(self, new_data:QObject):
        self._data.speed = new_data.speed
        self._data.angle = new_data.angle
        self._data.coord = new_data.coord
        self._data.qr_boxes = new_data.qr_boxes

        for i in range(len(new_data.qr_boxes)):

            if i >= len(self._qr_boxes_labels):
                self._qr_boxes_labels.append(QLabel(f'QR Box {i}: {self._data.qr_boxes[i]}'))
                self._layout_1.addWidget(self._qr_boxes_labels[i])
            else:
                self._qr_boxes_labels.setText(f'QR Box {i}: {self._data.qr_boxes[i]}')

        self._speed_label.setText(f'Speed: {new_data.speed}')
        self._coord_label.setText(f'Coordinates: {new_data.coord}')
        self._angle_label.setText(f'Angle: {new_data.angle}')