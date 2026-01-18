#include "Lolcat.h"
#include "Game.h"
#include "String.h"
#include "Platform.h"

cc_bool Lolcat_Auto;

static cc_bool Rand(void) {
    return (Stopwatch_Measure() & 1) != 0;
}

static char Up(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

void Lolcat_Transform(const cc_string* src, cc_string* dst) {
    dst->length = 0;

    for (int i = 0; i < src->length; i++) {
        char c = src->buffer[i];

        /* ===== WORD REPLACEMENTS ===== */

        if (i + 2 < src->length &&
            src->buffer[i] == 't' &&
            src->buffer[i+1] == 'h' &&
            src->buffer[i+2] == 'e') {
            String_AppendConst(dst, "TEH");
            i += 2;
            continue;
        }

        if (i + 2 < src->length &&
            src->buffer[i] == 'y' &&
            src->buffer[i+1] == 'o' &&
            src->buffer[i+2] == 'u') {
            String_AppendConst(dst, "U");
            i += 2;
            continue;
        }

        if (i + 2 < src->length &&
            src->buffer[i] == 'a' &&
            src->buffer[i+1] == 'r' &&
            src->buffer[i+2] == 'e') {
            String_AppendConst(dst, "R");
            i += 2;
            continue;
        }

        if (i + 3 < src->length &&
            src->buffer[i] == 'h' &&
            src->buffer[i+1] == 'a' &&
            src->buffer[i+2] == 'v' &&
            src->buffer[i+3] == 'e') {
            String_AppendConst(dst, "HAZ");
            i += 3;
            continue;
        }

        if (i + 5 < src->length &&
            src->buffer[i] == 'c' &&
            src->buffer[i+1] == 'h' &&
            src->buffer[i+2] == 'e' &&
            src->buffer[i+3] == 'e' &&
            src->buffer[i+4] == 's' &&
            src->buffer[i+5] == 'e') {
            String_AppendConst(dst, "CHEEZ");
            i += 5;
            continue;
        }

        if (i + 3 < src->length &&
            src->buffer[i] == 'w' &&
            src->buffer[i+1] == 'h' &&
            src->buffer[i+2] == 'a' &&
            src->buffer[i+3] == 't') {
            String_AppendConst(dst, "WAT");
            i += 3;
            continue;
        }

        if (i + 4 < src->length &&
            src->buffer[i] == 'h' &&
            src->buffer[i+1] == 'u' &&
            src->buffer[i+2] == 'm' &&
            src->buffer[i+3] == 'a' &&
            src->buffer[i+4] == 'n') {
            String_AppendConst(dst, "HOOMAN");
            i += 4;
            continue;
        }

        /* ===== LETTER CHAOS ===== */

        if (c == 's' && Rand()) c = 'z';
        if (c == 'S' && Rand()) c = 'Z';

        /* ===== FORCE UPPERCASE ===== */
        c = Up(c);
        String_Append(dst, c);
    }

    /* ===== LOLCAT NOISE ===== */
    int count = 3 + (Stopwatch_Measure() & 7);
    for (int i = 0; i < count; i++) {
        String_Append(dst, Rand() ? '!' : '1');
    }
}