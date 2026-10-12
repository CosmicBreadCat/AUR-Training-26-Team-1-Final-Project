from PySide6.QtWidgets import QMainWindow, QHBoxLayout, QVBoxLayout, QWidget
from aur_training_26_team_1_final_project.modes import MyModes
from aur_training_26_team_1_final_project.info import MyInfo
from aur_training_26_team_1_final_project.status import MyStatus
from aur_training_26_team_1_final_project.camera import MyCamera
from aur_training_26_team_1_final_project.controller import MyController
from aur_training_26_team_1_final_project.comm import MyCommunicationChannel


class Window(QMainWindow):
    
    def __init__(self):
    
        super().__init__()
        self.setWindowTitle("Console")
        self.resize(800, 600)

        self._modes = MyModes()
        self._info = MyInfo()
        self._status = MyStatus()
        self._camera = MyCamera()
        self._controller = MyController()
        self._comm = MyCommunicationChannel()

        self._central_layout = QVBoxLayout()
        self._central_layout.addWidget(self._camera)
        self._central_layout.addWidget(self._status)
        self._central_layout.setStretch(0, 3)
        self._central_layout.setStretch(1, 2)

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

        self._status.motion_started.connect(self._modes.motion_started)
        self._status.motion_stopped.connect(self._modes.motion_stopped)
        self._modes.autonomous_updated.connect(self._status.update_autonomous_mode)
        self._modes.colors_updated.connect(self._status.update_colors)
        self._modes.colors_updated.connect(self._camera.detector.update_colors)
        self._modes.boxes_picked.connect(self._status.update_picked_boxes)                        
        self._camera.fps_updated.connect(self._status.update_fps)
        self._camera.connection_updated.connect(self._status.update_connection_status)
        self._controller.controller_input_updated.connect(self._comm.get_input)     
