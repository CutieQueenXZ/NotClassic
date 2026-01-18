#include "Gemini.h"
#include "Platform.h"
#include "Chat.h"
#include <string.h>

static cc_socket gemini_sock = -1;
static cc_bool   gemini_waiting;

static cc_bool Gemini_Connect(void) {
    cc_sockaddr addrs[4];
    int count = 0;
    cc_string ip = String_FromConst("127.0.0.1");

    if (Socket_ParseAddress(&ip, 45700, addrs, &count) != 0 || count <= 0)
        return false;

    if (Socket_Create(&gemini_sock, &addrs[0], false) != 0)
        return false;

    if (Socket_Connect(gemini_sock, &addrs[0]) != 0) {
        Socket_Close(gemini_sock);
        gemini_sock = -1;
        return false;
    }

    return true;
}

cc_bool Gemini_Send(const cc_string* msg) {
    if (gemini_sock == -1) {
        if (!Gemini_Connect()) return false;
    }

    gemini_waiting = true;
    Socket_WriteAll(gemini_sock, msg->buffer, msg->length);
    Socket_WriteAll(gemini_sock, "\n", 1);
    return true;
}

void Gemini_Tick(void) {
    if (gemini_sock == -1 || !gemini_waiting) return;

    char buf[512];
    cc_uint32 read = 0;

    if (Socket_Read(gemini_sock, buf, sizeof(buf)-1, &read) != 0 || read == 0)
        return;

    buf[read] = 0;

    char* p = buf;
    while (1) {
        char* nl = strchr(p, '\n');
        if (!nl) break;
        *nl = 0;

        if (strcmp(p, "[END]") == 0) {
            gemini_waiting = false;
            break;
        }

        Chat_AddRaw(p);
        p = nl + 1;
    }
}