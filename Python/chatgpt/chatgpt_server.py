import socket
from openai import OpenAI

HOST = "127.0.0.1"
PORT = 45701
API_KEY = "Insert-API-Here"

SYSTEM_PROMPT = (
    "You are ChatGPT, user is from classicube server.\n"
    "Keep answers short. and dont use emojis, just because classicube doesnt support it\n"
    "Never use markdown. and dont use Apostrophe because broken\n"
    "Split long replies into multiple short lines.\n"
)

client = OpenAI(api_key=API_KEY)

def send_lines(conn, text):
    for line in text.split("\n"):
        line = line.strip()
        if line:
            conn.sendall((line + "\n").encode("utf-8", "replace"))
    conn.sendall(b"[END]\n")

print("[GPT] Server starting...")
srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind((HOST, PORT))
srv.listen(1)

while True:
    print("[GPT] Waiting for Classicube...")
    conn, addr = srv.accept()
    print("[GPT] Connected:", addr)

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

                print("[GPT] >", prompt)

                try:
                    r = client.chat.completions.create(
                        model="gpt-4o-mini",
                        messages=[
                            {"role": "system", "content": SYSTEM_PROMPT},
                            {"role": "user", "content": prompt},
                        ],
                        max_tokens=300,
                    )
                    text = r.choices[0].message.content
                except Exception as e:
                    text = "ChatGPT error: " + str(e)

                send_lines(conn, text)

    except Exception as e:
        print("[GPT] Error:", e)

    conn.close()
