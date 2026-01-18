import socket
from google import genai

HOST = "127.0.0.1"
PORT = 45700

API_KEY = "Your-API-here"
MODEL = "gemini-2.5-flash-lite"

SYSTEM_PROMPT = (
    "You're gemini ai"
    "Keep answers short, playful, and readable in chat. "
    "Never use markdown. No walls of text."
    "STRICT RULE: You must insert a line break (new line) after every 9 words you output."
    "Do not wait for a sentence to end. Count exactly 9 words, then hit enter."
)

client = genai.Client(api_key=API_KEY)

def send_lines(conn, text):
    for line in text.split("\n"):
        line = line.strip()
        if line:
            conn.sendall((line + "\n").encode("utf-8", "replace"))
    conn.sendall(b"[END]\n")

print("[Gemini] Server starting...")
srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind((HOST, PORT))
srv.listen(1)

while True:
    print("[Gemini] Waiting for Classicube...")
    conn, addr = srv.accept()
    print("[Gemini] Connected:", addr)

    buf = b""

    try:
        while True:
            data = conn.recv(4096)
            if not data:
                break

            buf += data

            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                prompt = line.decode("utf-8", "replace").strip()

                if not prompt:
                    continue

                print("[Gemini] >", prompt)

                try:
                    resp = client.models.generate_content(
                        model=MODEL,
                        contents=SYSTEM_PROMPT + "\nUser: " + prompt
                    )
                    text = getattr(resp, "text", "")[:1500]

                except Exception as e:
                    text = "Gemini error: " + str(e)

                send_lines(conn, text)

    except Exception as e:
        print("[Gemini] Error:", e)

    conn.close()
