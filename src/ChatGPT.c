#include "ChatGPT.h"
#include "Platform.h"
#include "Chat.h"
#include "String_.h"
#include <string.h>

static cc_socket gpt_sock = -1;
static cc_bool chatgpt_waiting = false;

static cc_bool ChatGPT_Connect(void) {
    cc_sockaddr addrs[4];
    int count = 0;
    cc_string ip = String_FromConst("127.0.0.1");

    if (Socket_ParseAddress(&ip, 45701, addrs, &count) != 0 || count <= 0)
        return false;

    if (Socket_Create(&gpt_sock, &addrs[0]) != 0)
        return false;

    if (Socket_Connect(gpt_sock, &addrs[0]) != 0) {
        Socket_Close(gpt_sock);
        gpt_sock = -1;
        return false;
    }
    return true;
}

cc_bool ChatGPT_Send(const cc_string* msg) {
    if (gpt_sock == -1) {
        if (!ChatGPT_Connect()) return false;
    }

    chatgpt_waiting = true;
    cc_uint32 wrote;
    Socket_Write(gpt_sock, (cc_uint8*)msg->buffer, msg->length, &wrote);
    cc_uint8 nl = '\n';
    Socket_Write(gpt_sock, &nl, 1, &wrote);
    return true;
}

void ChatGPT_Tick(void) {
    if (gpt_sock == -1 || !chatgpt_waiting) return;

    cc_uint8 buf[512];
    cc_uint32 read;

    if (Socket_Read(gpt_sock, buf, sizeof(buf) - 1, &read) != 0 || read == 0)
        return;

    buf[read] = 0;

    char* p = buf;
    while (1) {
        char* nl = strchr(p, '\n');
        if (!nl) break;
        *nl = 0;

        if (!strcmp(p, "[END]")) {
            chatgpt_waiting = false;
            break;
        }
        
        cc_string line; char buf2[600];
        String_InitArray(line, buf2);
        String_AppendConst(&line, "[ChatGPT] ");
        String_AppendConst(&line, p);
        Chat_Send(&line, false);
        p = nl + 1;
    }
}