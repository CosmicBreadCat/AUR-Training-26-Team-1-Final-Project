from PySide6.QtWidgets import QMainWindow, QHBoxLayout, QVBoxLayout, QWidget
from PySide6.QtCore import QThread
from aur_training_26_team_1_final_project.gui.modes import MyModes
from aur_training_26_team_1_final_project.gui.info import MyInfo
from aur_training_26_team_1_final_project.gui.status import MyStatus
from aur_training_26_team_1_final_project.cv.camera import MyCamera
from aur_training_26_team_1_final_project.comms.controller import MyController
from aur_training_26_team_1_final_project.comms.comm import MyCommunicationChannel

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
        
        self._thread = QThread()
        self._worker = MyCommunicationChannel()
        self._worker.moveToThread(self._thread)

        self._thread.started.connect(self._worker.start_communication)
        self._worker.finished.connect(self._thread.quit)
        self._worker.finished.connect(self._worker.deleteLater)
        self._thread.finished.connect(self._thread.deleteLater)

        self._thread.start()

        self._status.motion_started.connect(self._modes.motion_started)
        self._status.motion_stopped.connect(self._modes.motion_stopped)

        self._modes.autonomous_updated.connect(self._status.update_autonomous_mode)
        self._modes.colors_updated.connect(self._status.update_colors)
        self._modes.colors_updated.connect(self._camera.detector.update_colors)
        self._modes.boxes_picked.connect(self._status.update_picked_boxes)                        

        self._camera.fps_updated.connect(self._status.update_fps)
        self._camera.connection_updated.connect(self._status.update_connection_status)
        self._camera.data_updated.connect(self._info.update_qr_boxes_detected)

        self._controller.controller_input_updated.connect(self._worker.get_input)   

        self._worker.qr_box_dropped.connect(self._info.update_qr_boxes_dropped)
        self._worker.updated_odometry_data.connect(self._info.update_data)
