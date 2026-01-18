import cv2
import numpy as np
from PIL import Image

VIDEO_PATH = "badapple.mp4"   # your video
OUT_FILE   = "frames.txt"

WIDTH  = 40   # chat width
HEIGHT = 9    # chat lines
FPS_SKIP = 2  # increase to reduce spam (try 2–4)

WHITE = "#"
BLACK = "O"

cap = cv2.VideoCapture(VIDEO_PATH)
frame_id = 0

with open(OUT_FILE, "w", encoding="utf-8") as f:
    while True:
        ret, frame = cap.read()
        if not ret:
            break

        frame_id += 1
        if frame_id % FPS_SKIP != 0:
            continue

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        img = Image.fromarray(gray)
        img = img.resize((WIDTH, HEIGHT), Image.BILINEAR)

        pixels = np.array(img)

        f.write("FRAME\n")
        for y in range(HEIGHT):
            line = ""
            for x in range(WIDTH):
                line += WHITE if pixels[y][x] > 127 else BLACK
            f.write(line + "\n")
        f.write("\n")

cap.release()
print("Done! Frames saved to", OUT_FILE)
