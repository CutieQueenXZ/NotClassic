#include "BStream.h"
#include "Platform.h"
#include "Chat.h"
#include "Game.h"
#include "World.h"
#include "Block.h"
#include "String.h"
#include "Utils.h"
#include "Entity.h"   /* for LocalPlayer_SetPosition */
#include "Vectors.h"


/* Receive frames over TCP and paint them as blocks in the world (X/Z plane).
   Protocol:
     - connect to 127.0.0.1:port
     - each frame = h lines, newline-delimited ('\n')
     - each line length <= w (extra trimmed)
     - after h lines, frame is applied progressively across ticks
*/

#define BSTREAM_MAX_W 300
#define BSTREAM_MAX_H 300
#define BSTREAM_READ_BUF 2024
#define BSTREAM_APPLY_PER_TICK 9999

static cc_socket bstream_sock;
static cc_bool   bstream_active;

static int bstream_w, bstream_h;
static int bstream_x, bstream_y, bstream_z;
static int bstream_port;

/* receive state */
static char bstream_line[2024];
static int  bstream_lineLen;
static int  bstream_row;

/* frame buffer (chars) */
static char* bstream_frame;      /* size w*h */
static int   bstream_frameSize;

/* apply state */
static cc_bool bstream_haveFrame;
static int     bstream_applyIndex; /* 0..w*h-1 */

static const char* BSTREAM_CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";

static cc_uint8 Palette_BlockForIndex(int idx) {
    /* 64 blocks, mostly “safe” and useful. No bedrock/water/lava. */
    static const cc_uint8 blocks[64] = {
        /* 0-9 */
        BLOCK_AIR,        /* 0 */
        BLOCK_STONE,      /* 1 */
        BLOCK_GRASS,      /* 2 */
        BLOCK_DIRT,       /* 3 */
        BLOCK_COBBLE,     /* 4 */
        BLOCK_WOOD,       /* 5 */
        BLOCK_LOG,        /* 6 */
        BLOCK_LEAVES,     /* 7 */
        BLOCK_SAND,       /* 8 */
        BLOCK_GRAVEL,     /* 9 */

        /* A-J */
        BLOCK_GLASS,      /* A */
        BLOCK_SPONGE,     /* B */
        BLOCK_BRICK,      /* C */
        BLOCK_BOOKSHELF,  /* D */
        BLOCK_MOSSY_ROCKS,/* E */
        BLOCK_OBSIDIAN,   /* F */
        BLOCK_SANDSTONE,  /* G (CPE) */
        BLOCK_SNOW,       /* H (CPE) */
        BLOCK_ICE,        /* I (CPE) */
        BLOCK_STONE_BRICK,/* J (CPE) */

        /* K-T (ores + metals + slabs) */
        BLOCK_COAL_ORE,   /* K */
        BLOCK_IRON_ORE,   /* L */
        BLOCK_GOLD_ORE,   /* M */
        BLOCK_IRON,       /* N */
        BLOCK_GOLD,       /* O */
        BLOCK_SLAB,       /* P */
        BLOCK_DOUBLE_SLAB,/* Q */
        BLOCK_COBBLE_SLAB,/* R (CPE) */
        BLOCK_TNT,        /* S */
        BLOCK_CRATE,      /* T (CPE) */

        /* U-d (flowers/mushrooms + extra CPE) */
        BLOCK_DANDELION,  /* U */
        BLOCK_ROSE,       /* V */
        BLOCK_BROWN_SHROOM,/* W */
        BLOCK_RED_SHROOM, /* X */
        BLOCK_CERAMIC_TILE,/* Y (CPE) */
        BLOCK_MAGMA,      /* Z (CPE) */
        BLOCK_PILLAR,     /* a (CPE) */
        BLOCK_ROPE,       /* b (CPE) */
        BLOCK_FIRE,       /* c (CPE) */
        BLOCK_CYAN,       /* d */

        /* e-t (wool rainbow) */
        BLOCK_WHITE,      /* e */
        BLOCK_GRAY,       /* f */
        BLOCK_BLACK,      /* g */
        BLOCK_RED,        /* h */
        BLOCK_ORANGE,     /* i */
        BLOCK_YELLOW,     /* j */
        BLOCK_LIME,       /* k */
        BLOCK_GREEN,      /* l */
        BLOCK_TEAL,       /* m */
        BLOCK_AQUA,       /* n */
        BLOCK_CYAN,       /* o */
        BLOCK_BLUE,       /* p */
        BLOCK_INDIGO,     /* q */
        BLOCK_VIOLET,     /* r */
        BLOCK_MAGENTA,    /* s */
        BLOCK_PINK,       /* t */

        /* u-/ (more wool-ish CPE colors + brown/deep blue/turquoise/light pink + extras) */
        BLOCK_LIGHT_PINK, /* u (CPE) */
        BLOCK_FOREST_GREEN,/* v (CPE) */
        BLOCK_BROWN,      /* w (CPE) */
        BLOCK_DEEP_BLUE,  /* x (CPE) */
        BLOCK_TURQUOISE,  /* y (CPE) */
        BLOCK_IRON,       /* z (repeat ok) */
        BLOCK_GOLD,       /* + (repeat ok) */
        BLOCK_STONE       /* / (repeat ok) */
    };

    if (idx < 0) idx = 0;
    if (idx > 63) idx = 63;
    return blocks[idx];
}

static cc_uint8 Palette_BlockForChar(char c) {
    const char* p = BSTREAM_CHARS;
    for (int i = 0; p[i]; i++) {
        if (p[i] == c) return Palette_BlockForIndex(i);
    }

    /* old compatibility: allow hex too */
    if (c >= '0' && c <= '9') return Palette_BlockForIndex(c - '0');
    if (c >= 'A' && c <= 'F') return Palette_BlockForIndex(10 + (c - 'A'));
    if (c >= 'a' && c <= 'f') return Palette_BlockForIndex(10 + (c - 'a'));

    /* treat space/dot as air */
    if (c == ' ' || c == '.' || c == '_') return BLOCK_AIR;

    /* fallback */
    return BLOCK_STONE;
}

static void BStream_ResetFrame(void) {
    bstream_lineLen    = 0;
    bstream_row        = 0;
    bstream_haveFrame  = false;
    bstream_applyIndex = 0;
}

static void BStream_StopInternal(const char* reason) {
    if (!bstream_active) return;

    Socket_Close(bstream_sock);
    bstream_active = false;

    if (reason) {
        cc_string msg; char buf[160];
        String_InitArray(msg, buf);
        String_AppendConst(&msg, "&7[BStream] ");
        String_AppendConst(&msg, reason);
        Chat_Add(&msg);
    }
}

void BStream_Stop(void) { BStream_StopInternal("stopped"); }

static cc_bool BStream_Connect(void) {
    cc_sockaddr addrs[4];
    int count = 0;
    cc_string ip = String_FromConst("127.0.0.1");

    if (Socket_ParseAddress(&ip, bstream_port, addrs, &count) != 0 || count <= 0) return false;

    if (Socket_Create(&bstream_sock, &addrs[0], false) != 0) return false;
    if (Socket_Connect(bstream_sock, &addrs[0]) != 0) {
        Socket_Close(bstream_sock);
        return false;
    }
    return true;
}

static cc_bool BStream_AllocFrame(void) {
    int size = bstream_w * bstream_h;
    if (size <= 0) return false;

    if (bstream_frame && bstream_frameSize != size) {
        Mem_Free(bstream_frame);
        bstream_frame = NULL;
        bstream_frameSize = 0;
    }
    if (!bstream_frame) {
        bstream_frame = (char*)Mem_Alloc(size, 1, "bstream_frame");
        if (!bstream_frame) return false;
        bstream_frameSize = size;
    }
    Mem_Set(bstream_frame, ' ', size);
    return true;
}

cc_bool BStream_Start(int w, int h, int x, int y, int z, int port) {
    if (bstream_active) BStream_StopInternal("restarting");

    if (w <= 0 || h <= 0 || w > BSTREAM_MAX_W || h > BSTREAM_MAX_H) {
        Chat_AddRaw("&c[BStream] bad size");
        return true;
    }

    bstream_w = w; bstream_h = h;
    bstream_x = x; bstream_y = y; bstream_z = z;
    bstream_port = port;

    if (!BStream_AllocFrame()) {
        Chat_AddRaw("&c[BStream] out of memory");
        return false;
    }
    if (!BStream_Connect()) {
        Chat_AddRaw("&c[BStream] connect failed (is python running?)");
        return false;
    }

    bstream_active = true;
    BStream_ResetFrame();
    return true;
}

static void BStream_StoreLine(const char* line, int len) {
    int rowOff, copy;
    if (!bstream_frame) return;

    if (len < 0) len = 0;
    if (len > bstream_w) len = bstream_w;

    rowOff = bstream_row * bstream_w;
    Mem_Set(bstream_frame + rowOff, ' ', bstream_w);
    copy = len;
    if (copy) Mem_Copy(bstream_frame + rowOff, line, copy);

    bstream_row++;
    if (bstream_row >= bstream_h) {
        bstream_haveFrame  = true;
        bstream_applyIndex = 0;
        bstream_row = 0; /* next frame starts immediately */
    }
}

static void BStream_HandleByte(char c) {
    if (c == '\r') return;

    if (c == '\n') {
        BStream_StoreLine(bstream_line, bstream_lineLen);
        bstream_lineLen = 0;
        return;
    }

    if (bstream_lineLen < (int)(sizeof(bstream_line) - 1)) {
        bstream_line[bstream_lineLen++] = c;
    }
}

static void BStream_ApplyBatch(void) {
    if (!bstream_haveFrame || !bstream_frame) return;

    int total = bstream_w * bstream_h;
    int end   = bstream_applyIndex + BSTREAM_APPLY_PER_TICK;
    if (end > total) end = total;

    for (int idx = bstream_applyIndex; idx < end; idx++) {
        int dx = idx % bstream_w;
        int dz = idx / bstream_w;

        char c = bstream_frame[idx];
        cc_uint8 block = Palette_BlockForChar(c);

        int wx = bstream_x + dx;
        int wy = bstream_y;
        int wz = bstream_z + dz;

        if (!World_Contains(wx, wy, wz)) continue;
        if (World_GetBlock(wx, wy, wz) == block) continue;

        /* now place block */
        Game_ChangeBlock(wx, wy, wz, block);
    }

    bstream_applyIndex = end;

    if (bstream_applyIndex >= total) {
        bstream_haveFrame  = false;
        bstream_applyIndex = 0;
    }
}

void BStream_Tick(void) {
    cc_uint8 buf[BSTREAM_READ_BUF];
    cc_uint32 read = 0;

    if (!bstream_active) return;

    if (Socket_Read(bstream_sock, buf, sizeof(buf), &read) != 0) {
        BStream_StopInternal("socket closed/error");
        return;
    }

    for (cc_uint32 i = 0; i < read; i++) {
        BStream_HandleByte((char)buf[i]);
    }

    BStream_ApplyBatch();
}
