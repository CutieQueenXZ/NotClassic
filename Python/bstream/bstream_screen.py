import socket
import time
import mss
import numpy as np
import cv2

PORT = 45679
W = 100 #Reminder: if u want to change the size, please edit this whatever size if u want
H = 100 #example:like if its 100, then "/client bstream start 100 100 X Y Z" or if did u change to 200, then replace 100 to 200

# 16-color palette (RGB)
PALETTE = np.array([
    (255,255,255), (160,160,160), (0,0,0), (255,192,203),
    (255,0,255), (128,0,255), (75,0,130), (0,0,255),
    (0,255,255), (0,200,200), (0,255,0), (150,255,0),
    (255,255,0), (255,165,0), (255,0,0), (200,200,255)
], dtype=np.uint8)

CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";

def rgb_to_hexchar(rgb):
    diff = PALETTE - rgb
    dist = np.sum(diff * diff, axis=1)
    return CHARS[np.argmin(dist)]

print("[BStream] Waiting for Classicube...")

server = socket.socket()
server.bind(("0.0.0.0", PORT))
server.listen(1)
conn, addr = server.accept()

print("[BStream] Client connected:", addr)

with mss.mss() as sct:
    monitor = sct.monitors[1]

    while True:
        img = np.array(sct.grab(monitor))[:,:,:3]
        img = cv2.resize(img, (W, H), interpolation=cv2.INTER_AREA)

        frame = []
        for y in range(H):
            row = ""
            for x in range(W):
                row += rgb_to_hexchar(img[y,x])
            frame.append(row)

        data = ("\n".join(frame) + "\n").encode()
        conn.sendall(data)

        time.sleep(0.05)
