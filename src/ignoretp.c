#include "ignoretp.h"
#include "Chat.h"

cc_bool IgnoreTP_Enabled = false;

void IgnoreTP_Init(void) {
    IgnoreTP_Enabled = false;
}

cc_bool IgnoreTP_ShouldBlock(const struct Entity* ent, const Vec3* newPos) {
    if (!IgnoreTP_Enabled) return false;
    if (!ent || !newPos) return false;

    /* Only block local player */
    if (Entities.CurPlayer && ent != &Entities.CurPlayer->Base) return false;

    float dx = ent->Position.x - newPos->x;
    float dy = ent->Position.y - newPos->y;
    float dz = ent->Position.z - newPos->z;

    float distSq = dx*dx + dy*dy + dz*dz;

    /* Ignore small corrections, block big teleports */
    if (distSq > 16.0f) return true;

    return false;
}