#ifndef CC_BSTREAM_H
#define CC_BSTREAM_H
#include "Core.h"

cc_bool BStream_Start(int w, int h, int x, int y, int z, int port);
void    BStream_Stop(void);
void    BStream_Tick(void);

#endif
