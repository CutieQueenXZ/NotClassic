#ifndef CC_GEMINI_H
#define CC_GEMINI_H

#pragma once
#include "String.h"

cc_bool Gemini_Send(const cc_string* msg);
void    Gemini_Tick(void);

#endif