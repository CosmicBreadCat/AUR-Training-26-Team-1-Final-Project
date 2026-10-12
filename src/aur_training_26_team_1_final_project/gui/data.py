from PySide6.QtCore import QObject, Property, Signal

class MyData(QObject):
    speed_changed = Signal()
    coord_changed = Signal()
    angle_changed = Signal()
    qr_boxes_changed = Signal()

    def __init__(self, parent):
        super().__init__(parent)

        self._speed = 0
        self._coord = 0
        self._angle = 0
        self._qr_boxes = []

    @Property(int, notify=speed_changed)
    def speed(self):
        return self._speed

    @speed.setter
    def speed(self, new:int):
        self._speed = new
        self.speed_changed.emit()

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

    @Property(int, notify=qr_boxes_changed)
    def qr_boxes(self):
        return self._qr_boxes

    @qr_boxes.setter
    def qr_boxes(self, new:int):
        self._qr_boxes = new
        self.qr_boxes_changed.emit()