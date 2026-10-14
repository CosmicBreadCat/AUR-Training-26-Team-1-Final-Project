from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QLineEdit
from PySide6.QtGui import QColor, QPalette, QIntValidator
from PySide6.QtCore import Signal
from functools import partial

class MyModes(QWidget):
    colors_updated = Signal(list)
    autonomous_updated = Signal(int)
    boxes_picked = Signal(int)

    def __init__(self):

        super().__init__()
        print('Modes Section has successfully appeared')

        #Background Color
        self.setAutoFillBackground(True)
        palette = self.palette()
        palette.setColor(QPalette.ColorRole.Window, QColor("#3e3e3e"))
        self.setPalette(palette)

        #Validator for LineEdits of hue
        validator = QIntValidator(0, 255)

        #Easy Boxes Settings
        self._easy = QVBoxLayout()
        self._easy_1 = QHBoxLayout()
        self._easy_2 = QHBoxLayout()

        self._easy_text = QLabel()
        self._easy_text.setText('Easy Boxes')
        self._easy_btn = QPushButton()
        self._easy_btn.setText('Change Mode')

        self._easy_lower_hue = QLineEdit(placeholderText='Lower Hue (0 - 359)')
        self._easy_upper_hue = QLineEdit(placeholderText='Upper Hue (0 - 359)')
        self._easy_lower_hue.setValidator(validator)
        self._easy_upper_hue.setValidator(validator)

        self._easy_1.addWidget(self._easy_text)
        self._easy_1.addWidget(self._easy_btn)
        self._easy_2.addWidget(self._easy_lower_hue)
        self._easy_2.addWidget(self._easy_upper_hue)
        self._easy.addLayout(self._easy_1)
        self._easy.addLayout(self._easy_2)

        #Hard Boxes Settings
        self._hard = QVBoxLayout()
        self._hard_1 = QHBoxLayout()
        self._hard_2 = QHBoxLayout()
    
        self._hard_text = QLabel()
        self._hard_text.setText('Hard Boxes')
        self._hard_btn = QPushButton()
        self._hard_btn.setText('Change Mode')

        self._hard_lower_hue = QLineEdit(placeholderText='Lower Hue (0 - 359)')
        self._hard_upper_hue = QLineEdit(placeholderText='Upper Hue (0 - 359)')
        self._hard_lower_hue.setValidator(validator)
        self._hard_upper_hue.setValidator(validator)

        self._hard_1.addWidget(self._hard_text)
        self._hard_1.addWidget(self._hard_btn)
        self._hard_2.addWidget(self._hard_lower_hue)
        self._hard_2.addWidget(self._hard_upper_hue)
        self._hard.addLayout(self._hard_1)
        self._hard.addLayout(self._hard_2)

        #QR Boxes Settings    
        self._qr = QVBoxLayout()
        self._qr_1 = QHBoxLayout()
        self._qr_2 = QHBoxLayout()
    
        self._qr_text = QLabel()
        self._qr_text.setText('QR Boxes')
        self._qr_btn = QPushButton()
        self._qr_btn.setText('Change Mode')

        self._qr_lower_hue = QLineEdit(placeholderText='Lower Hue (0 - 359)')
        self._qr_upper_hue = QLineEdit(placeholderText='Upper Hue (0 - 359)')
        self._qr_lower_hue.setValidator(validator)
        self._qr_upper_hue.setValidator(validator)

        self._qr_1.addWidget(self._qr_text)
        self._qr_1.addWidget(self._qr_btn)
        self._qr_2.addWidget(self._qr_lower_hue)
        self._qr_2.addWidget(self._qr_upper_hue)
        self._qr.addLayout(self._qr_1)
        self._qr.addLayout(self._qr_2)

        #Update Colors Button
        self._update_colors_btn = QPushButton('Update Colors')

        self._box_modes = QVBoxLayout()

        self._box_modes_label = QLabel('Box Modes')
        self._box_modes_label.setStyleSheet(
            'font-size: 16px; font-weight: bold;'
        )

        self._box_modes.addWidget(self._box_modes_label)
        self._box_modes.addLayout(self._easy)
        self._box_modes.addLayout(self._hard)
        self._box_modes.addLayout(self._qr)
        self._box_modes.addWidget(self._update_colors_btn)

        #Autonomous Modes Settings
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

        #Overall Container
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
        self._container.setStretch(2, 3)

        self._title_layout.addStretch()
        self._box_modes.addStretch()
        self._autonomous_modes.addStretch()

        #Function Triggers

        self._update_colors_btn.clicked.connect(self._update_colors)

        self._no_autonomous.clicked.connect(partial(self._update_autonomous, 0))
        self._semi_autonomous.clicked.connect(partial(self._update_autonomous, 1))
        self._fully_autonomous.clicked.connect(partial(self._update_autonomous, 2))

        self._easy_btn.clicked.connect(partial(self._pick_box, 0))
        self._hard_btn.clicked.connect(partial(self._pick_box, 1))
        self._qr_btn.clicked.connect(partial(self._pick_box, 2))

    def motion_started(self):
        self._no_autonomous.setEnabled(False)
        self._semi_autonomous.setEnabled(False)
        self._fully_autonomous.setEnabled(False)
        self._update_colors_btn.setEnabled(False)

    def motion_stopped(self):
        self._no_autonomous.setEnabled(True)
        self._semi_autonomous.setEnabled(True)
        self._fully_autonomous.setEnabled(True)
        self._update_colors_btn.setEnabled(True)

    def _get_text(self, wid:QLineEdit):
        txt = wid.text()
        if len(txt) > 0 and 0 <= int(txt) and int(txt) <= 359:
            return int(txt) // 2
        else:
            #Don't change default color
            return -1

    def _update_colors(self):
        colors = []

        colors.append(self._get_text(self._easy_lower_hue))
        colors.append(self._get_text(self._easy_upper_hue))
        colors.append(self._get_text(self._hard_lower_hue))
        colors.append(self._get_text(self._hard_upper_hue))
        colors.append(self._get_text(self._qr_lower_hue))
        colors.append(self._get_text(self._qr_upper_hue))

        print(f"New hue Colors: {colors}")
        self.colors_updated.emit(colors)

    def _update_autonomous(self, mode:int):
        print(f"Autonomous Mode: {mode}")
        self.autonomous_updated.emit(mode)

    def _pick_box(self, mode:int):
        print(f"Picked boxes: {mode}")
        self.boxes_picked.emit(mode)