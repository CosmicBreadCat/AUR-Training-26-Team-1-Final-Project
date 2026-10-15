import pygame
from PySide6.QtCore import QObject, Signal, QTimer

class MyController(QObject):
    controller_input_updated = Signal(list)

    BUTTON_NAMES = {
        0: "A",
        1: "B",
        2: "X",
        3: "Y",
        4: "Left Bumper (LB)",
        5: "Right Bumper (RB)",
        6: "View / Back",
        7: "Menu / Start",
        8: "Left Stick Click (L3)",
        9: "Right Stick Click (R3)",
        10: "Xbox Guide",
        11: "Share",
    }

    AXIS_NAMES = {
        0: "Left Stick X",
        1: "Left Stick Y",
        2: "Right Stick X", 
        3: "Right Stick Y",
        4: "Left Trigger (LT)",
        5: "Right Trigger (RT)",
    }

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

        # 0 -> XBOX Controller, 1 -> PS3 Controller
        self._used_controller = 1 

    def _get_data(self):

        try: 
            # Process internal events to refresh controller states
            pygame.event.pump()

            inputs = []

            # 1. Analog Axes (Left Stick X/Y, Right Stick X/Y, LT, RT)
            for i in range(self._controller.get_numaxes()):
                inputs.append(round(self._controller.get_axis(i), 3))
                
            # 2. D-Pad (Hat directions: X, Y)
            for i in range(self._controller.get_numhats()):
                hat_x, hat_y = self._controller.get_hat(i)
                inputs.extend([hat_x, hat_y])
                
            # 3. Buttons (A, B, X, Y, LB, RB, Back, Start, L3, R3, Guide)
            for i in range(self._controller.get_numbuttons()):
                inputs.append(self._controller.get_button(i))

            # Fix triggers range
            inputs[4] = (inputs[4]+1.0)/2.0
            inputs[5] = (inputs[5]+1.0)/2.0

            if self._used_controller == 1:
                # Left X -> 0
                # Left Y -> 1 (1 to -1) changed to (-1 to 1)
                # Right X -> 3 
                # Right Y -> 2 (1 to -1) changed to (-1 to 1) 
                # Left Trigger -> 10 (Digital in other controller)
                # Right Trigger -> 11 (Digital in other controller)
                # D-Pad X -> 4 (0, 0.5, 1) changed to (-1, 0, 1)
                # D-Pad Y -> 5 (0, 0.5, 1) changed to (-1, 0, 1)
                # Button A -> 8
                # Button B -> 7
                # Button X -> 9 
                # Button Y -> 6
                # Left Bumper -> 12
                # Right Bumper -> 13

                mapped_inputs = inputs.copy()

                mapped_inputs[2], mapped_inputs[3] = inputs[3], inputs[2]

                mapped_inputs[1] *= -1
                mapped_inputs[3] *= -1

                mapped_inputs[4] = inputs[10]
                mapped_inputs[5] = inputs[11]

                mapped_inputs[6] = inputs[4] * 2 - 1
                mapped_inputs[7] = inputs[5] * 2 - 1

                mapped_inputs[8] = inputs[8]
                mapped_inputs[9] = inputs[7]
                mapped_inputs[10] = inputs[9]
                mapped_inputs[11] = inputs[6]
                mapped_inputs[12] = inputs[12]
                mapped_inputs[13] = inputs[13]


                inputs = mapped_inputs


            for i in range(len(inputs)):
                if abs(inputs[i]) < 0.015:
                    inputs[i] = 0

            # For debugging
            print(inputs)

            self.controller_input_updated.emit(inputs)

        except pygame.error as e:
            print(f"Controller error: {e}")
            self._timer.stop()
