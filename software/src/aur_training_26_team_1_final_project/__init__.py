from PySide6.QtWidgets import QApplication
from aur_training_26_team_1_final_project.gui.window import Window

def main() -> None:
    print("GUI is open!")
    app = QApplication()
    win = Window()
    app.exec()