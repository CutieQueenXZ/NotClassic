    import socket
import requests
import unicodedata

HOST = "127.0.0.1"
PORT = 5000

AZURE_KEY = "API-KEY"
AZURE_REGION = "eastus"

def to_ascii(text):
    return (
        unicodedata
        .normalize("NFKD", text)
        .encode("ascii", "ignore")
        .decode("ascii")
    )

def translate(lang, text):
    endpoint = (
        "https://api.cognitive.microsofttranslator.com/translate"
        f"?api-version=3.0&to={lang}"
    )

    headers = {
        "Ocp-Apim-Subscription-Key": AZURE_KEY,
        "Ocp-Apim-Subscription-Region": AZURE_REGION,
        "Content-Type": "application/json"
    }

    body = [{"text": text}]
    r = requests.post(endpoint, headers=headers, json=body, timeout=5)
    r.raise_for_status()

    translated = r.json()[0]["translations"][0]["text"]
    return to_ascii(translated)

print("Translator listening...")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen(5)

    while True:
        conn, addr = s.accept()
        with conn:
            data = conn.recv(2048)
            if not data:
                continue

            raw = data.decode("utf-8").strip()

            # Parse "lang|text"
            if "|" in raw:
                lang, text = raw.split("|", 1)
            else:
                lang = "es"
                text = raw

            try:
                out = translate(lang, text)
            except Exception as e:
                out = "Translation failed"

            conn.sendall(out.encode("utf-8"))

