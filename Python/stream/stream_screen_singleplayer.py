import socket
import time
import numpy as np
from PIL import ImageGrab, Image

HOST = "127.0.0.1"
PORT = 45678

ASCII = "O-=+*#%$/"

WIDTH  = 64   # chat width
HEIGHT = 32   # chat height
FPS = 10

# fallback capture box (change if needed :D )
BBOX = {
    "left": 100,
    "top": 100,
    "width": 640,
    "height": 480
}

def frame_to_ascii(img: Image.Image):
    img = img.resize((WIDTH, HEIGHT))
    img = img.convert("L")

    pixels = np.asarray(img)

    scale = (pixels.astype(np.float32) / 255.0) * (len(ASCII) - 1)
    scale = scale.astype(np.int32)

    lines = []
    for row in scale:
        line = "".join(ASCII[i] for i in row)
        lines.append(line)

    return lines

def main():
    print("[+] Starting stream server...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((HOST, PORT))
    sock.listen(1)

    print("[+] Waiting for Classicube client...")
    conn, addr = sock.accept()
    print("[+] Client connected:", addr)

    try:
        while True:
            img = ImageGrab.grab(bbox=(
                BBOX["left"],
                BBOX["top"],
                BBOX["left"] + BBOX["width"],
                BBOX["top"] + BBOX["height"],
            ))

            lines = frame_to_ascii(img)

            for line in lines:
                conn.sendall((line + "\n").encode("utf-8"))

            time.sleep(1 / FPS)

    except KeyboardInterrupt:
        print("\n[!] Stopped")
    finally:
        conn.close()
        sock.close()

if __name__ == "__main__":
    main()
