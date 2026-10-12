import cv2
import numpy as np
from PySide6.QtCore import QObject, Property, Signal
from PySide6.QtGui import QImage, QPixmap

class MyDetector(QObject):
    new_data_sent = Signal()

    def __init__(self):
        super().__init__()

        print("")
        #Hue Colors
        self._colors = [160, 10, 35, 85, 100, 140]
        
        self._qr_detector = cv2.QRCodeDetector()

    def qimage_to_cv2(self, frame: QImage):
        frame = frame.convertToFormat(QImage.Format.Format_RGBA8888)
        
        width = frame.width()
        height = frame.height()
        bytes_per_line = frame.bytesPerLine()
        
        ptr = frame.constBits()
        arr = np.array(ptr).reshape((height, bytes_per_line))[:, :width * 4]
        rgba = arr.reshape((height, width, 4))
    
        frame_bgr = cv2.cvtColor(rgba, cv2.COLOR_RGBA2BGR)
        return frame_bgr

    def cv2_to_qimage(self, frame):
        rgb_img = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        height, width, _ = rgb_img.shape
        bytes_per_line = 3 * width
        q_img = QImage(
            rgb_img.data,
            width,
            height,
            bytes_per_line,
            QImage.Format.Format_RGB888,
        )
        return q_img.copy()

    def get_mask(self, idx, frame):

        if self._colors[idx*2] > self._colors[idx*2+1]:

            lower_mask = np.array([self._colors[idx*2], 80, 80])
            upper_mask = np.array([179, 255, 255])

            lower_mask2 = np.array([0, 80, 80])
            upper_mask2 = np.array([self._colors[idx*2+1], 255, 255])
            
            return cv2.inRange(frame, lower_mask, upper_mask) | cv2.inRange(frame, lower_mask2, upper_mask2)

        lower_mask = np.array([self._colors[idx*2], 80, 80])
        upper_mask = np.array([self._colors[idx*2+1], 255, 255])
        return cv2.inRange(frame, lower_mask, upper_mask)

    def getQRCodeReader(self, frame):
        decoded_texts = []
        decoded_data = []
    
        success, decoded_info, points, _ = self._qr_detector.detectAndDecodeMulti(frame)
        if success:
            for i in range(len(decoded_info)):
                decoded_data.append({"text": decoded_info[i], "points": points[i]})
            decoded_texts = [text for text in decoded_info if text]
        else:
            text, points, _ = self._qr_detector.detectAndDecode(frame)
            if text:
                decoded_texts.append(text)
                decoded_data.append({"text": text, "points": points})

        return decoded_data
    

    def get_data(self, qframe:QImage):

        frame = self.qimage_to_cv2(qframe)
        
        frame = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

        masks = []
        boxes = []

        kernel = np.ones((5, 5), np.uint8)

        for i in range(3):
            new_frame = frame.copy()
            masks.append(self.get_mask(i, new_frame))
            masks[i] = cv2.morphologyEx(masks[i], cv2.MORPH_OPEN, kernel)

            contours, hierarchy = cv2.findContours(masks[i], cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

            for contour in contours:
                area = cv2.contourArea(contour)
                if area < 200:
                    continue

                perimeter = cv2.arcLength(contour, True)
                points = cv2.approxPolyDP(contour, 0.02 * perimeter, True)

                M = cv2.moments(points)
                if M["m00"] == 0:
                    cx = cy = None
                else:
                    cx = int(M["m10"] / M["m00"])
                    cy = int(M["m01"] / M["m00"])

                boxes.append({"cx": cx, "cy": cy, "type": i, "points": points})

                range_ = (self._colors[i*2+1]-self._colors[i*2]+180)%180

                frame = cv2.drawContours(frame, [points], 0, ((self._colors[i*2]+range_//2)%180, 255, 255), 2)

        frame = cv2.cvtColor(frame, cv2.COLOR_HSV2BGR)

        decoded_data = self.getQRCodeReader(frame)

        for data in decoded_data:
            points = data["points"]
            points = np.int32(points).reshape((-1, 1, 2))
            data["points"] = points.astype(int)
            # For debugging purposes
            # frame = cv2.drawContours(frame, [points], 0, (0, 255, 255), 2)
        
        results = {
            "qr_readings": decoded_data,
            # For debugging purposes
            # "frame": frame,
            "boxes": boxes
        }
        
        self.new_data_sent.emit()

        return results

    def update_colors(self, new:list):
        for i in range(6):
            if new[i] != -1:
                self._colors[i] = new[i]