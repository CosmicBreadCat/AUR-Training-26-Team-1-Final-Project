from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QLineEdit
from PySide6.QtGui import QColor, QPalette, QIntValidator
from PySide6.QtCore import Qt, Signal

class MyModes(QWidget):

    def __init__(self):

        super().__init__()
        print('Modes Section has successfully appeared')
            
        self.setAutoFillBackground(True)
        palette = self.palette()
        palette.setColor(QPalette.ColorRole.Window, QColor("#3e3e3e"))
        self.setPalette(palette)

        self._easy = QVBoxLayout()
        self._easy_1 = QHBoxLayout()
        self._easy_2 = QHBoxLayout()

        self._easy_text = QLabel()
        self._easy_text.setText('Easy Boxes')
        self._easy_btn = QPushButton()
        self._easy_btn.setText('Change Mode')
        self._easy_lower_hsv = QLineEdit()
        self._easy_upper_hsv = QLineEdit()

        self._easy_1.addWidget(self._easy_text)
        self._easy_1.addWidget(self._easy_btn)
        self._easy_2.addWidget(self._easy_lower_hsv)
        self._easy_2.addWidget(self._easy_upper_hsv)
        self._easy.addLayout(self._easy_1)
        self._easy.addLayout(self._easy_2)

        self._hard = QVBoxLayout()
        self._hard_1 = QHBoxLayout()
        self._hard_2 = QHBoxLayout()
    
        self._hard_text = QLabel()
        self._hard_text.setText('Hard Boxes')
        self._hard_btn = QPushButton()
        self._hard_btn.setText('Change Mode')
        self._hard_lower_hsv = QLineEdit()
        self._hard_upper_hsv = QLineEdit()

        self._hard_1.addWidget(self._hard_text)
        self._hard_1.addWidget(self._hard_btn)
        self._hard_2.addWidget(self._hard_lower_hsv)
        self._hard_2.addWidget(self._hard_upper_hsv)
        self._hard.addLayout(self._hard_1)
        self._hard.addLayout(self._hard_2)
    
        self._qr = QVBoxLayout()
        self._qr_1 = QHBoxLayout()
        self._qr_2 = QHBoxLayout()
    
        self._qr_text = QLabel()
        self._qr_text.setText('QR Boxes')
        self._qr_btn = QPushButton()
        self._qr_btn.setText('Change Mode')
        self._qr_lower_hsv = QLineEdit()
        self._qr_upper_hsv = QLineEdit()

        self._qr_1.addWidget(self._qr_text)
        self._qr_1.addWidget(self._qr_btn)
        self._qr_2.addWidget(self._qr_lower_hsv)
        self._qr_2.addWidget(self._qr_upper_hsv)
        self._qr.addLayout(self._qr_1)
        self._qr.addLayout(self._qr_2)

        self._box_modes = QVBoxLayout()

        self._box_modes_label = QLabel('Box Modes')
        self._box_modes_label.setStyleSheet(
            'font-size: 16px; font-weight: bold;'
        )

        self._box_modes.addWidget(self._box_modes_label)
        self._box_modes.addLayout(self._easy)
        self._box_modes.addLayout(self._hard)
        self._box_modes.addLayout(self._qr)
        
        self._autonomous_label = QLabel('Autonomous Modes')
        self._autonomous_label.setStyleSheet(
            'font-size: 16px; font-weight: bold;'
        )

        self._no_autonomous = QPushButton('No Autonomous Mode')
        self._semi_autonomous = QPushButton('Semi-Autonomous Mode')
        self._fully_autonomous = QPushButton('Fully-Autonomous Mode')

        self._autonomous_modes = QVBoxLayout()

        self._autonomous_modes.addWidget(self._autonomous_label)
        self._autonomous_modes.addWidget(self._no_autonomous)
        self._autonomous_modes.addWidget(self._semi_autonomous)
        self._autonomous_modes.addWidget(self._fully_autonomous)

        self._container = QVBoxLayout(self)
        self._modes_label = QLabel('Modes')
        self._modes_label.setStyleSheet(
            'font-size: 20px; font-weight: bold;'
        )

        self._title_layout = QVBoxLayout()
        self._title_layout.addWidget(self._modes_label)

        self._container.addLayout(self._title_layout)
        self._container.addLayout(self._box_modes)
        self._container.addLayout(self._autonomous_modes)
        
        self._container.setStretch(0, 1)
        self._container.setStretch(1, 5)
        self._container.setStretch(2, 5)

        self._title_layout.addStretch()
        self._box_modes.addStretch()
        self._autonomous_modes.addStretch()