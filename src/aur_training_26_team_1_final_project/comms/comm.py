from aur_training_26_team_1_final_project.comms.robotlink import RobotLink
from PySide6.QtCore import QObject
import math

class MyCommunicationChannel(QObject):
    # MODES = ("STOP", "WHEEL_VEL", "TWIST", "MOVE", "TURN", "SQUARE", "APPROACH")

    def __init__(self):
        
        super().__init__()

        self._mode = None

        # 4 Analog inputs then 4 digital inputs

        self._data = []

        self._robotlink = RobotLink()
        self._set_twist_mode()

    def get_input(self, data:list):
        self._data = data

        self._run_twist_mode()

    def _get_vel_resultant(self, v_x, v_y):

        vel = math.sqrt(v_x ** 2 + v_y ** 2)
        
        omega = 0
        if v_x == 0:
            if v_y > 0:
                omega = math.acos(-1) / 2.0
            else:
                omega = 3.0 * math.acos(-1) / 2.0
        else:
            omega = math.atan(v_y/v_x)

        vel = round(vel, 3)
        omega = round(omega, 3)
        return vel, omega

    def _set_twist_mode(self):
        self._mode = "TWIST"
        self._robotlink.set_mode("TWIST")
    
    def _run_twist_mode(self):
        vel, omega = self._get_vel_resultant(self._data[0], self._data[1])

        self._robotlink.set_twist(vel, omega)