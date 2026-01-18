#ifndef CHAT_MACROS_H
#define CHAT_MACROS_H

#include "String.h"

void ChatMacros_Expand(cc_string* text);
void ChatMacros_Add(const cc_string* trigger, const cc_string* replacement);

#endif
