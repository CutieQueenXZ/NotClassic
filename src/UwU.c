#include "UwU.h"
#include "Platform.h"
#include "String.h"

cc_bool UwU_Auto;

static cc_bool Rand(void) {
    return (Stopwatch_Measure() & 1) != 0;
}

void UwU_Transform(const cc_string* src, cc_string* dst) {
    dst->length = 0;

    for (int i = 0; i < src->length; i++) {
        char c = src->buffer[i];
        char n = (i+1 < src->length) ? src->buffer[i+1] : 0;

        /* r/l -> w */
        if (c=='r') c='w';
        if (c=='R') c='W';
        if (c=='l') c='w';
        if (c=='L') c='W';

        /* n + vowel -> ny */
        if ((c=='n' || c=='N') &&
           (n=='a'||n=='e'||n=='i'||n=='o'||n=='u')) {
            String_Append(dst,c);
            String_Append(dst,(c=='N')?'Y':'y');
            continue;
        }

        String_Append(dst,c);
    }

    if (Rand()) {
        String_AppendConst(dst, Rand() ? " owo" : " uwu");
    }
}