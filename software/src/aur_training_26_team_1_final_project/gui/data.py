from PySide6.QtCore import QObject, Property, Signal

class MyData(QObject):
    linear_speed_changed = Signal()
    angular_speed_changed = Signal()
    coord_changed = Signal()
    angle_changed = Signal()
    qr_boxes_dropped_changed = Signal()
    qr_boxes_detected_changed = Signal()
    qr_boxes_on_camera_changed = Signal()

    def __init__(self, parent):
        super().__init__(parent)

        self._linear_speed = 0
        self._angular_speed = 0
        self._coord = 0
        self._angle = 0
        self._qr_boxes_dropped = []
        self._qr_boxes_detected = []
        self._qr_boxes_on_camera = []

    @Property(int, notify=linear_speed_changed)
    def linear_speed(self):
        return self._linear_speed

    @linear_speed.setter
    def linear_speed(self, new:int):
        self._linear_speed = new
        self.linear_speed_changed.emit()
        
    @Property(int, notify=angular_speed_changed)
    def angular_speed(self):
        return self._angular_speed

    @angular_speed.setter
    def angular_speed(self, new:int):
        self._angular_speed = new
        self.angular_speed_changed.emit()

    @Property(int, notify=coord_changed)
    def coord(self):
        return self._coord

    @coord.setter
    def coord(self, new:int):
        self._coord = new
        self.coord_changed.emit()
        
    @Property(int, notify=angle_changed)
    def angle(self):
        return self._angle

    @angle.setter
    def angle(self, new:int):
        self._angle = new
        self.angle_changed.emit()

    @Property(list, notify=qr_boxes_dropped_changed)
    def qr_boxes_dropped(self):
        return self._qr_boxes_dropped

    @qr_boxes_dropped.setter
    def qr_boxes_dropped(self, new:list):
        self._qr_boxes_dropped = new
        self.qr_boxes_dropped_changed.emit()

    @Property(list, notify=qr_boxes_detected_changed)
    def qr_boxes_detected(self):
        return self._qr_boxes_detected

    @qr_boxes_detected.setter
    def qr_boxes_detected(self, new:list):
        self._qr_boxes_detected = new
        self.qr_boxes_detected_changed.emit()

    @Property(list, notify=qr_boxes_on_camera_changed)
    def qr_boxes_on_camera(self):
        return self._qr_boxes_on_camera

    @qr_boxes_on_camera.setter
    def qr_boxes_on_camera(self, new:list):
        self._qr_boxes_on_camera = new
        self.qr_boxes_on_camera_changed.emit()