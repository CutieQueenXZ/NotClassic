#ifndef IGNORETP_H
#define IGNORETP_H

#include "Core.h"
#include "Entity.h"
#include "Vectors.h"   /* defines Vec3 */

extern cc_bool IgnoreTP_Enabled;

cc_bool IgnoreTP_ShouldBlock(const struct Entity* ent, const Vec3* newPos);
void IgnoreTP_Init(void);

#endif