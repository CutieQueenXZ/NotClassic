#pragma once
#include "String_.h"
#include "Core.h"

extern cc_bool Lolcat_Auto; 

cc_bool NC_GetLolcat(void);
void NC_SetLolcat(cc_bool v);

void Lolcat_Transform(const cc_string* src, cc_string* dst);