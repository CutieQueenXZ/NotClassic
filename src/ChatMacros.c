#include "ChatMacros.h"
#include "Platform.h"
#include "Constants.h"
#include "Core.h"
#include <string.h>

#define MAX_MACROS 32

struct ChatMacro {
    cc_string trigger;
    cc_string replacement;
};

static struct ChatMacro macros[MAX_MACROS];
static int macro_count;

/* simple lowercase char */
static char ToLower(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

/* case-insensitive equals */
static cc_bool String_EqualsCaseless(const cc_string* a, const cc_string* b) {
    if (a->length != b->length) return false;
    for (int i = 0; i < a->length; i++) {
        if (ToLower(a->buffer[i]) != ToLower(b->buffer[i])) return false;
    }
    return true;
}

void ChatMacros_Expand(cc_string* text) {
    for (int i = 0; i < macro_count; i++) {
        if (String_EqualsCaseless(text, &macros[i].trigger)) {
            *text = macros[i].replacement;
            return;
        }
    }
}

void ChatMacros_Add(const cc_string* trigger, const cc_string* replacement) {
    if (macro_count >= MAX_MACROS) return;

    struct ChatMacro* m = &macros[macro_count++];

    m->trigger.length = trigger->length;
    m->trigger.buffer = Mem_Alloc(trigger->length, 1, "macro");
    Mem_Copy(m->trigger.buffer, trigger->buffer, trigger->length);

    m->replacement.length = replacement->length;
    m->replacement.buffer = Mem_Alloc(replacement->length, 1, "macro");
    Mem_Copy(m->replacement.buffer, replacement->buffer, replacement->length);
}
