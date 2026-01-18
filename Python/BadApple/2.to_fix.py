import os

INPUT_DIR = "frames_txt"
OUTPUT_DIR = "frames_fixed" #i forgor if i am right... because its long ago to?

CHUNK = 5

os.makedirs(OUTPUT_DIR, exist_ok=True)

def space_chunks(line, n):
    return " ".join(line[i:i+n] for i in range(0, len(line), n))

files = sorted(f for f in os.listdir(INPUT_DIR) if f.endswith(".txt"))

for fname in files:
    in_path = os.path.join(INPUT_DIR, fname)
    out_path = os.path.join(OUTPUT_DIR, fname)

    with open(in_path, "r", encoding="utf-8") as f:
        lines = f.read().splitlines()

    fixed = [space_chunks(line, CHUNK) for line in lines]

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(fixed))

print(f"YES! Fixed {len(files)} frames → {OUTPUT_DIR}/")
