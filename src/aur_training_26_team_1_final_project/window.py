from PySide6.QtWidgets import QMainWindow, QHBoxLayout, QVBoxLayout, QWidget
from aur_training_26_team_1_final_project.modes import MyModes
from aur_training_26_team_1_final_project.info import MyInfo
from aur_training_26_team_1_final_project.status import MyStatus
from aur_training_26_team_1_final_project.camera import MyCamera


class Window(QMainWindow):
    
    def __init__(self):
    
        super().__init__()
        self.setWindowTitle("Console")
        self.resize(800, 600)

        self._modes = MyModes()
        self._info = MyInfo()
        self._status = MyStatus()
        self._camera = MyCamera()

        self._central_layout = QVBoxLayout()
        self._central_layout.addWidget(self._camera)
        self._central_layout.addWidget(self._status)

        self._layout = QHBoxLayout()
        self._layout.addWidget(self._info)
        self._layout.addLayout(self._central_layout)
        self._layout.addWidget(self._modes)
        self._layout.setStretch(0, 1)
        self._layout.setStretch(1, 3)
        self._layout.setStretch(2, 1)

        self._widget = QWidget()
        self._widget.setLayout(self._layout)

        self.setCentralWidget(self._widget)

        self.show()
                        
                
