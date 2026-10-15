from PySide6.QtWidgets import QWidget
from PySide6.QtQuickWidgets import QQuickWidget
from PySide6.QtCore import QUrl, QObject
from aur_training_26_team_1_final_project.assets import get_asset

class Compass(QQuickWidget):
    def __init__(self, widget_filepath: str, 
                data_bridge: QObject | None = None,
                parent: QWidget | None = None):
        super().__init__(parent)

        self.setInitialProperties({"compassData": data_bridge})
        
        self.setSource(QUrl.fromLocalFile(get_asset(widget_filepath)))
        
        # self.setResizeMode(QQuickWidget.ResizeMode.SizeRootObjectToView)
        self.setResizeMode(QQuickWidget.ResizeMode.SizeViewToRootObject)