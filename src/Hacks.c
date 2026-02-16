#include "Hacks.h"

cc_bool ClientHax_Enabled = false;

void Hacks_ApplyClient(struct HacksComp* h) {
    if (!h) return;

    if (ClientHax_Enabled) {
        h->Enabled   = true;
        h->CanFly    = true;
        h->CanNoclip = true;
        h->CanSpeed  = true;
        #if defined(CC_BUILD_ANDROID)
        TouchOnscreen_UpdateButton(s, btn);
	    TouchScreen_Refresh();
        #endif
    } else {
        h->Flying   = false;
        h->Noclip   = false;
        h->Speeding = false;

        h->CanFly    = false;
        h->CanNoclip = false;
        h->CanSpeed  = false;
    }
}