#ifndef CC_CUBOID_H
#define CC_CUBOID_H

#include "Core.h"
#include "Vectors.h"

/* tick function called every frame */
void CuboidCommand_Tick(void);

/* state flag (read-only outside Commands.c) */
extern cc_bool CuboidCommand_Active;

#endif
