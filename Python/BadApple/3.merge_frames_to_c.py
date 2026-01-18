import os

INPUT_DIR = "frames_fixed"
OUTPUT_FILE = "BadAppleFrames.h"
HEIGHT = 12

frames = sorted(f for f in os.listdir(INPUT_DIR) if f.endswith(".txt"))

with open(OUTPUT_FILE, "w", encoding="utf-8") as out:
    out.write("#pragma once\n\n")
    out.write(f"#define BADAPPLE_FRAME_COUNT {len(frames)}\n")
    out.write(f"#define BADAPPLE_FRAME_HEIGHT {HEIGHT}\n\n")
    out.write("static const char* BADAPPLE_FRAMES[BADAPPLE_FRAME_COUNT][BADAPPLE_FRAME_HEIGHT] = {\n")

    for frame in frames:
        out.write("    {\n")
        with open(os.path.join(INPUT_DIR, frame), "r", encoding="utf-8") as f:
            lines = f.read().splitlines()[:HEIGHT]

        for line in lines:
            safe = line.replace("\\", "\\\\").replace("\"", "\\\"")
            out.write(f"        \".{safe}\",\n")

        out.write("    },\n")

    out.write("};\n")

print("Done. Header generated safely! enjoy...?")
