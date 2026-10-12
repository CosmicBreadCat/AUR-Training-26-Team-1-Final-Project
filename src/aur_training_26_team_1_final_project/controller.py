import sys
import time
import pygame
from PySide6.QtCore import QObject, Property, Signal, QTimer

class MyController(QObject):
    controller_input_updated = Signal(list)

    def __init__(self):
        super().__init__()

        self.FREQ = 50 # 50 Hz
        pygame.init()
        pygame.joystick.init()

        count = pygame.joystick.get_count()
        if count == 0:
            print("No controller found. Please connect one and rerun.")
            return

        self._controller = pygame.joystick.Joystick(0)
        self._controller.init()

        # For debugging
        # print(f"Connected: {controller.get_name()}")
        # print(f"Axes (Analog): {controller.get_numaxes()}")
        # print(f"Buttons (Digital): {controller.get_numbuttons()}")
        # print(f"Hats / D-Pads: {controller.get_numhats()}")
        # print("-" * 50)

        self._timer = QTimer()
        self._timer.setInterval(1000 // self.FREQ)
        self._timer.timeout.connect(self._get_data)
        self._timer.start()

    def _get_data(self):

        try:
            while True:
                # Process internal events to refresh controller states
                pygame.event.pump()

                data = []

                # 1. Analog Inputs (Joysticks & Triggers, range typically -1.0 to 1.0)
                axes = [round(self._controller.get_axis(i), 3) for i in range(self._controller.get_numaxes())]

                for i in axes:
                    data.append(i)

                # 2. Digital Inputs (Buttons: 0 = Released, 1 = Pressed)
                buttons = [self._controller.get_button(i) for i in range(self._controller.get_numbuttons())]

                for i in buttons:
                    data.append(i)

                # 3. D-Pad / Hats (returns a tuple like (x, y) with -1, 0, or 1)
                hats = [self._controller.get_hat(i) for i in range(self._controller.get_numhats())]

                # For debugging
                # print(f"\rAxes (Analog): {axes} | Buttons: {buttons} | Hats: {hats}", end="", flush=True)

                self.controller_input_updated.emit(data)
                    
                time.sleep(1.0/self.FREQ)

        except KeyboardInterrupt:
            print("\nController Disconnected")
        finally:
            pygame.quit()
