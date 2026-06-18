#include "Core.h"

#if !defined CC_BUILD_NETWORKING
#include "Game.h"
#include "Protocol.h"

struct _ProtocolData Protocol;
cc_bool cpe_needD3Fix = false;

cc_bool Env_IgnoreServer = false;
int Server_MaxViewDistance = DEFAULT_MAX_VIEWDIST;

char displayMotdBuffer[STRING_SIZE];
cc_string DisplayMOTD;
cc_bool LocalMotd = false;

void CPE_SendExtInfo(int extsCount) { }
void CPE_SendPlayerClick(int button, cc_bool pressed, cc_uint8 targetId, struct RayTracer* t) { }
void CPE_SendNotifyAction(int action, cc_uint16 value) { }
void CPE_SendNotifyPositionAction(int action, int x, int y, int z) { }
void CPE_SendPluginMessage(cc_uint8 channel, cc_uint8* data) { }

void Classic_BuildLogin(struct LoginPacket* pkt) { }
void Classic_BuildChat(const cc_string* text, cc_bool partial, struct ChatPacket* pkt) { }
void Classic_SendSetBlock(int x, int y, int z, cc_bool place, BlockID block) { }

void Motd_Tick(void) { }
void Protocol_Tick(void) { }

static void Protocol_NullInit(void) { }

struct IGameComponent Protocol_Component = {
	Protocol_NullInit,
	NULL,
	Protocol_NullInit
};
#endif
