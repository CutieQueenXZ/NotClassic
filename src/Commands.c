#include "Commands.h"
#include "Chat.h"
#include "String.h"
#include "Event.h"
#include "Game.h"
#include "Logger.h"
#include "Server.h"
#include "World.h"
#include "Inventory.h"
#include "Entity.h"
#include "Window.h"
#include "Graphics.h"
#include "Funcs.h"
#include "Block.h"
#include "EnvRenderer.h"
#include <ctype.h>
#include "Utils.h"
#include "TexturePack.h"
#include "Options.h"
#include "Drawer2D.h"
#include "Audio.h"
#include "Http.h"
#include "Platform.h"
#include "CuboidCommand.h"
#include "Cuboid.h"
#include "Options.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "Entity.h"
#include <stdio.h>
#include "Camera.h"
#include "PackedCol.h"
#include "Core.h"
#include "InputHandler.h"
#include <stdlib.h>
#include "UwU.h"
#include "BStream.h"
#include "Gemini.h"
#include "ExtMath.h"
#include "Picking.h"
#include "Camera.h"
#include "Hacks.h"

static cc_bool posfly_active;
static cc_uint64 posfly_last;
static cc_bool posfly_active = false;
static struct ScheduledTask* posfly_task = NULL;

#define COMMANDS_PREFIX "/client"
#define COMMANDS_PREFIX_SPACE "/client "
static struct ChatCommand* cmds_head;
static struct ChatCommand* cmds_tail;

void Commands_Register(struct ChatCommand* cmd) {
	LinkedList_Append(cmd, cmds_head, cmds_tail);
}

/*########################################################################################################################*
*------------------------------------------------------External Functions---------------------------------------------------*
*#########################################################################################################################*/

void wait(double seconds) {
	clock_t start = clock();
	while ((double)(clock() - start) / CLOCKS_PER_SEC < seconds) {

	}
}

void nop() {}

/*########################################################################################################################*
*------------------------------------------------------Command handling---------------------------------------------------*
*#########################################################################################################################*/
static struct ChatCommand* Commands_FindMatch(const cc_string* cmdName) {
	struct ChatCommand* match = NULL;
	struct ChatCommand* cmd;
	cc_string name;

	for (cmd = cmds_head; cmd; cmd = cmd->next) 
	{
		name = String_FromReadonly(cmd->name);
		if (String_CaselessEquals(&name, cmdName)) return cmd;
	}

	for (cmd = cmds_head; cmd; cmd = cmd->next) 
	{
		name = String_FromReadonly(cmd->name);
		if (!String_CaselessStarts(&name, cmdName)) continue;

		if (match) {
			Chat_Add1("&e/client: Multiple commands found that start with: \"&f%s&e\".", cmdName);
			return NULL;
		}
		match = cmd;
	}

	if (!match) {
		Chat_Add1("&e/client: Unrecognised command: \"&f%s&e\".", cmdName);
		Chat_AddRaw("&e/client: Type &a/client &efor a list of commands.");
	}
	return match;
}

static void Commands_PrintDefault(void) {
	cc_string str; char strBuffer[STRING_SIZE];
	struct ChatCommand* cmd;
	cc_string name;

	Chat_AddRaw("&eList of client commands:");
	String_InitArray(str, strBuffer);

	for (cmd = cmds_head; cmd; cmd = cmd->next) {
		name = String_FromReadonly(cmd->name);

		if ((str.length + name.length + 2) > str.capacity) {
			Chat_Add(&str);
			str.length = 0;
		}
		String_AppendString(&str, &name);
		String_AppendConst(&str, ", ");
	}

	if (str.length) { Chat_Add(&str); }
	Chat_AddRaw("&eTo see help for a command, type /client help [cmd name]");
}

cc_bool Commands_Execute(const cc_string* input) {
	static const cc_string prefixSpace = String_FromConst(COMMANDS_PREFIX_SPACE);
	static const cc_string prefix      = String_FromConst(COMMANDS_PREFIX);
	cc_string text;

	struct ChatCommand* cmd;
	int offset, count;
	cc_string name, value;
	cc_string args[50];

	if (String_CaselessStarts(input, &prefixSpace)) { 
		/* /client [command] [args] */
		offset = prefixSpace.length;
	} else if (String_CaselessEquals(input, &prefix)) { 
		/* /client */
		offset = prefix.length;
	} else if (Server.IsSinglePlayer && String_CaselessStarts(input, &prefix)) {
		/* /client[command] [args] */
		offset = prefix.length;
	} else if (Server.IsSinglePlayer && input->length && input->buffer[0] == '/') {
		/* /[command] [args] */
		offset = 1;
	} else {
		return false;
	}

	text = String_UNSAFE_SubstringAt(input, offset);
	/* Check for only / or /client */
	if (!text.length) { Commands_PrintDefault(); return true; }

	String_UNSAFE_Separate(&text, ' ', &name, &value);
	cmd = Commands_FindMatch(&name);
	if (!cmd) return true;

	if (cmd->flags & COMMAND_FLAG_UNSPLIT_ARGS) {
		/* argsCount = 0 if value.length is 0, 1 otherwise */
		cmd->Execute(&value, value.length != 0);
	} else {
		count = String_UNSAFE_Split(&value, ' ', args, Array_Elems(args));
		cmd->Execute(args,   value.length ? count : 0);
	}
	return true;
}


/*########################################################################################################################*
*------------------------------------------------------Simple commands----------------------------------------------------*
*#########################################################################################################################*/
static void HelpCommand_Execute(const cc_string* args, int argsCount) {
	struct ChatCommand* cmd;
	int i;

	if (!argsCount) { Commands_PrintDefault(); return; }
	cmd = Commands_FindMatch(args);
	if (!cmd) return;

	for (i = 0; i < Array_Elems(cmd->help); i++) {
		if (!cmd->help[i]) continue;
		Chat_AddRaw(cmd->help[i]);
	}
}

static struct ChatCommand HelpCommand = {
	"Help", HelpCommand_Execute,
	COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client help [command name]",
		"&eDisplays the help for the given command.",
	}
};

static void GpuInfoCommand_Execute(const cc_string* args, int argsCount) {
	char buffer[7 * STRING_SIZE];
	cc_string str, line;
	String_InitArray(str, buffer);
	Gfx_GetApiInfo(&str);
	
	while (str.length) {
		String_UNSAFE_SplitBy(&str, '\n', &line);
		if (line.length) Chat_Add1("&a%s", &line);
	}
}

static struct ChatCommand GpuInfoCommand = {
	"GpuInfo", GpuInfoCommand_Execute,
	COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client gpuinfo",
		"&eDisplays information about your GPU.",
	}
};

static void RenderTypeCommand_Execute(const cc_string* args, int argsCount) {
	int flags;
	if (!argsCount) {
		Chat_AddRaw("&e/client: &cYou didn't specify a new render type."); return;
	}

	flags = EnvRenderer_CalcFlags(args);
	if (flags >= 0) {
		EnvRenderer_SetMode(flags);
		Options_Set(OPT_RENDER_TYPE, args);
		Chat_Add1("&e/client: &fRender type is now %s.", args);
	} else {
		Chat_Add1("&e/client: &cUnrecognised render type &f\"%s\"&c.", args);
	}
}

static struct ChatCommand RenderTypeCommand = {
	"RenderType", RenderTypeCommand_Execute,
	COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client rendertype [normal/legacy/fast]",
		"&bnormal: &eDefault render mode, with all environmental effects enabled",
		"&blegacy: &eSame as normal mode, &cbut is usually slightly slower",
		"   &eIf you have issues with clouds and map edges disappearing randomly, use this mode",
		"&bfast: &eSacrifices clouds, fog and overhead sky for faster performance",
	}
};

static void ResolutionCommand_Execute(const cc_string* args, int argsCount) {
	int width, height;
	if (argsCount < 2) {
		Chat_Add4("&e/client: &fCurrent resolution is %i@%f2 x %i@%f2", 
				&Window_Main.Width, &DisplayInfo.ScaleX, &Window_Main.Height, &DisplayInfo.ScaleY);
	} else if (!Convert_ParseInt(&args[0], &width) || !Convert_ParseInt(&args[1], &height)) {
		Chat_AddRaw("&e/client: &cWidth and height must be integers.");
	} else if (width <= 0 || height <= 0) {
		Chat_AddRaw("&e/client: &cWidth and height must be above 0.");
	} else {
		Window_SetSize(width, height);
		Options_SetInt(OPT_WINDOW_WIDTH,  (int)(width  / DisplayInfo.ScaleX));
		Options_SetInt(OPT_WINDOW_HEIGHT, (int)(height / DisplayInfo.ScaleY));
	}
}

static struct ChatCommand ResolutionCommand = {
	"Resolution", ResolutionCommand_Execute,
	0,
	{
		"&a/client resolution [width] [height]",
		"&ePrecisely sets the size of the rendered window.",
	}
};

static void ModelCommand_Execute(const cc_string* args, int argsCount) {
	if (argsCount) {
		Entity_SetModel(&Entities.CurPlayer->Base, args);
	} else {
		Chat_AddRaw("&e/client model: &cYou didn't specify a model name.");
	}
}

static struct ChatCommand ModelCommand = {
	"Model", ModelCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client model [name]",
		"&bnames: &echibi, chicken, creeper, human, pig, sheep",
		"&e       skeleton, spider, zombie, sit, <numerical block id>",
	}
};

static void SkinCommand_Execute(const cc_string* args, int argsCount) {
	if (argsCount) {
		Entity_SetSkin(&Entities.CurPlayer->Base, args);
	} else {
		Chat_AddRaw("&e/client skin: &cYou didn't specify a skin name.");
	}
}

static struct ChatCommand SkinCommand = {
	"Skin", SkinCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client skin [name]",
		"&eChanges to the skin to the given player",
		"&a/client skin [url]",
		"&eChanges skin to a URL linking directly to a .PNG",
	}
};

static void ClearDeniedCommand_Execute(const cc_string* args, int argsCount) {
	int count = TextureUrls_ClearDenied();
	Chat_Add1("Removed &e%i &fdenied texture pack URLs.", &count);
}

static struct ChatCommand ClearDeniedCommand = {
	"ClearDenied", ClearDeniedCommand_Execute,
	COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client cleardenied",
		"&eClears the list of texture pack URLs you have denied",
	}
};

static void MotdCommand_Execute(const cc_string* args, int argsCount) {
	if (Server.IsSinglePlayer) {
		Chat_AddRaw("&eThis command can only be used in multiplayer.");
		return;
	}
	Chat_Add1("&eName: &f%s", &Server.Name);
	Chat_Add1("&eMOTD: &f%s", &Server.MOTD);
}

static struct ChatCommand MotdCommand = {
	"Motd", MotdCommand_Execute,
	COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client motd",
		"&eDisplays the server's name and MOTD."
	}
};

/*#######################################################################################################################*
*-------------------------------------------------------PlaceCommand-----------------------------------------------------*
*########################################################################################################################*/

static void PlaceCommand_Execute(const cc_string* args, int argsCount) {
	cc_string name;
	cc_uint8 off;
	int block;
	IVec3 pos;
	
	if (argsCount == 2) {
		Chat_AddRaw("&eToo few arguments.");
		return;
	}
	
	block = !argsCount || argsCount == 3 ? Inventory_SelectedBlock : Block_Parse(&args[0]);
	
	if (block == -1) {
		Chat_AddRaw("&eCould not parse block.");
		return;
	}
	if (block > Game_Version.MaxCoreBlock && !Block_IsCustomDefined(block)) {
		Chat_Add1("&eThere is no block with id \"%i\".", &block); 
		return;
	}
	
	if (argsCount > 2) {
		off = argsCount == 4;
		if (!Convert_ParseInt(&args[0 + off], &pos.x) || !Convert_ParseInt(&args[1 + off], &pos.y) || !Convert_ParseInt(&args[2 + off], &pos.z)) {
			Chat_AddRaw("&eCould not parse coordinates.");
			return;
		}
	} else {
		IVec3_Floor(&pos, &Entities.CurPlayer->Base.Position);
	}
	
	if (!World_Contains(pos.x, pos.y, pos.z)) {
		Chat_AddRaw("&eCoordinates are outside the world boundaries.");
		return;
	}
	
	Game_ChangeBlock(pos.x, pos.y, pos.z, block);
	name = Block_UNSAFE_GetName(block);
	Chat_Add4("&eSuccessfully placed %s block at (%i, %i, %i).", &name, &pos.x, &pos.y, &pos.z);
}

static struct ChatCommand PlaceCommand = {
	"Place", PlaceCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY,
	{
		"&a/client place [block] [x y z]",
		"&ePlaces block at [x y z].",
		"&eIf no block is provided, held block is used.",
		"&eIf no coordinates are provided, current",
		"&e coordinates are used."
	}
};

/*########################################################################################################################*
*-------------------------------------------------------DrawOpCommand-----------------------------------------------------*
*#########################################################################################################################*/
static IVec3 drawOp_mark1, drawOp_mark2;
static cc_bool drawOp_persist, drawOp_hooked, drawOp_hasMark1;
static const char* drawOp_name;
static void (*drawOp_Func)(IVec3 min, IVec3 max);

static void DrawOpCommand_BlockChanged(void* obj, IVec3 coords, BlockID old, BlockID now);
static void DrawOpCommand_ResetState(void) {
	if (drawOp_hooked) {
		Event_Unregister_(&UserEvents.BlockChanged, NULL, DrawOpCommand_BlockChanged);
		drawOp_hooked = false;
	}

	drawOp_hasMark1 = false;
}

static void DrawOpCommand_Begin(void) {
	cc_string msg; char msgBuffer[STRING_SIZE];
	String_InitArray(msg, msgBuffer);

	String_Format1(&msg, "&e%c: &fPlace or delete a block.", drawOp_name);
	Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_1);

	Event_Register_(&UserEvents.BlockChanged, NULL, DrawOpCommand_BlockChanged);
	drawOp_hooked = true;
}


static void DrawOpCommand_Execute(void) {
	IVec3 min, max;

	IVec3_Min(&min, &drawOp_mark1, &drawOp_mark2);
	IVec3_Max(&max, &drawOp_mark1, &drawOp_mark2);
	if (!World_Contains(min.x, min.y, min.z)) return;
	if (!World_Contains(max.x, max.y, max.z)) return;

	drawOp_Func(min, max);
}

static void DrawOpCommand_BlockChanged(void* obj, IVec3 coords, BlockID old, BlockID now) {
	cc_string msg; char msgBuffer[STRING_SIZE];
	String_InitArray(msg, msgBuffer);
	Game_UpdateBlock(coords.x, coords.y, coords.z, old);

	if (!drawOp_hasMark1) {
		drawOp_mark1    = coords;
		drawOp_hasMark1 = true;

		String_Format4(&msg, "&e%c: &fMark 1 placed at (%i, %i, %i), place mark 2.", 
						drawOp_name, &coords.x, &coords.y, &coords.z);
		Chat_AddOf(&msg, MSG_TYPE_CLIENTSTATUS_1);
	} else {
		drawOp_mark2 = coords;

		DrawOpCommand_Execute();
		DrawOpCommand_ResetState();

		if (!drawOp_persist) {			
			Chat_AddOf(&String_Empty, MSG_TYPE_CLIENTSTATUS_1);
		} else {
			DrawOpCommand_Begin();
		}
	}
}

static const cc_string yes_string = String_FromConst("yes");
static void DrawOpCommand_ExtractPersistArg(cc_string* value) {
	drawOp_persist = false;
	if (!String_CaselessEnds(value, &yes_string)) return;

	drawOp_persist = true; 
	value->length  -= 3;
	String_UNSAFE_TrimEnd(value);
}

static int DrawOpCommand_ParseBlock(const cc_string* arg) {
	int block = Block_Parse(arg);

	if (block == -1) {
		Chat_Add2("&e%c: &c\"%s\" is not a valid block name or id.", drawOp_name, arg); 
		return -1;
	}

	if (block > Game_Version.MaxCoreBlock && !Block_IsCustomDefined(block)) {
		Chat_Add2("&e%c: &cThere is no block with id \"%s\".", drawOp_name, arg); 
		return -1;
	}
	return block;
}


/*########################################################################################################################*
*-------------------------------------------------------CuboidCommand-----------------------------------------------------*
*#########################################################################################################################*/

static cc_bool cuboid_active;
static IVec3 cuboid_min, cuboid_max, cuboid_cur;
static BlockID cuboid_place;
static int cuboid_block;

static cc_uint64 cuboid_lastTime;
static cc_uint64 cuboid_delay_us = 40000;

static void CuboidCommand_Draw(IVec3 min, IVec3 max) {
    cuboid_min = min;
    cuboid_max = max;
    cuboid_cur = min;

    cuboid_place = cuboid_block == -1 ?
        Inventory_SelectedBlock : (BlockID)cuboid_block;

    cuboid_active = true;
    cuboid_lastTime = Stopwatch_Measure();
}

void CuboidCommand_Tick(void) {
    cc_uint64 now;
    cc_uint64 delay_us;

    if (!cuboid_active) return;

    delay_us = (cc_uint64)Options_CuboidDelayMS * 1000;

    now = Stopwatch_Measure();
    if (Stopwatch_ElapsedMicroseconds(cuboid_lastTime, now) < delay_us)
        return;

    cuboid_lastTime = now;

    Game_ChangeBlock(
        cuboid_cur.x,
        cuboid_cur.y,
        cuboid_cur.z,
        cuboid_place
    );

    struct Entity* e = &Entities.CurPlayer->Base;
        struct LocationUpdate u;
        Vec3 p;

        p.x = cuboid_cur.x + 0.5f;
        p.y = cuboid_cur.y + 1.6f;
        p.z = cuboid_cur.z + 0.5f;

        u.flags = LU_HAS_POS;
        u.pos   = p;
        e->VTABLE->SetLocation(e, &u);


    cuboid_cur.x++;
    if (cuboid_cur.x > cuboid_max.x) {
        cuboid_cur.x = cuboid_min.x;
        cuboid_cur.z++;
    }
    if (cuboid_cur.z > cuboid_max.z) {
        cuboid_cur.z = cuboid_min.z;
        cuboid_cur.y++;
    }
    if (cuboid_cur.y > cuboid_max.y) {
        cuboid_active = false;
        Chat_AddRaw("&aCuboid complete.");
    }
}


static void Cuboid_TeleportNear(int x, int y, int z) {
    struct Entity* e = &Entities.CurPlayer->Base;
    struct LocationUpdate u;
    Vec3 pos;

    pos.x = x + 0.5f;
    pos.y = y + 1.6f;
    pos.z = z + 0.5f;

    u.flags = LU_HAS_POS;
    u.pos   = pos;

    e->VTABLE->SetLocation(e, &u);
}

static void MovePlayerNearBlock(int x, int y, int z) {
    struct Entity* e = &Entities.CurPlayer->Base;
    struct LocationUpdate update;
    Vec3 v;

    v.x = x + 0.0f;
    v.y = y + 0.0f;
    v.z = z + 0.0f;

    update.flags = LU_HAS_POS;
    update.pos   = v;

    e->VTABLE->SetLocation(e, &update);
}

static void CuboidCommand_Execute(const cc_string* args, int argsCount) {
    cc_string value = *args;

    DrawOpCommand_ResetState();
    drawOp_name = "Cuboid";
    drawOp_Func = CuboidCommand_Draw;

    DrawOpCommand_ExtractPersistArg(&value);
    cuboid_block = -1;

    if (value.length) {
        cuboid_block = DrawOpCommand_ParseBlock(&value);
        if (cuboid_block == -1) return;
    }

    DrawOpCommand_Begin();
}	

static struct ChatCommand CuboidCommand = {
    "Cuboid", CuboidCommand_Execute,
    COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
    {
        "&a/client cuboid [block] [persist]",
        "&eVery slow cuboid (anti-kick safe)",
        "&ePlaces 1 block every 250ms"
    }
};

/*########################################################################################################################*
*-------------------------------------------------------OCuboidCommand-----------------------------------------------------*
*#########################################################################################################################*/
static int ocuboid_block;

static void OCuboidCommand_Draw(IVec3 min, IVec3 max) {
	BlockID toPlace;
	int x, y, z;

	toPlace = (BlockID)ocuboid_block;
	if (ocuboid_block == -1) toPlace = Inventory_SelectedBlock;

	for (y = min.y; y <= max.y; y++) {
		for (z = min.z; z <= max.z; z++) {
			for (x = min.x; x <= max.x; x++) {
				Game_ChangeBlock(x, y, z, toPlace);
			}
		}
	}
}

static void OCuboidCommand_Execute(const cc_string* args, int argsCount) {
	cc_string value = *args;

	DrawOpCommand_ResetState();
	drawOp_name = "OCuboid";
	drawOp_Func = OCuboidCommand_Draw;

	DrawOpCommand_ExtractPersistArg(&value);
	ocuboid_block = -1; 

	if (value.length) {
		ocuboid_block = DrawOpCommand_ParseBlock(&value);
		if (ocuboid_block == -1) return;
	}

	DrawOpCommand_Begin();
}

static struct ChatCommand OCuboidCommand = {
	"OCuboid", OCuboidCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"this is old cuboid command, usable for singleplayer!"
		"&a/client cuboid [block] [persist]",
		"&eFills the 3D rectangle between two points with [block].",
		"&eIf no block is given, uses your currently held block.",
		"&e  If persist is given and is \"yes\", then the command",
		"&e  will repeatedly cuboid, without needing to be typed in again.",
	}
};

/*########################################################################################################################*
*------------------------------------------------------SCuboidCommand----------------------------------------------------*
*#########################################################################################################################*/

extern cc_uint32 Options_CuboidDelayMS;

static cc_bool scuboid_active;
static IVec3 scuboid_min, scuboid_max, scuboid_cur;
static BlockID scuboid_place;
static int scuboid_block;

static cc_uint64 scuboid_lastTime;
#define SCUBOID_DEFAULT_DELAY_MS 40

static void SCuboidCommand_Draw(IVec3 min, IVec3 max) {
	scuboid_min = min;
	scuboid_max = max;
	scuboid_cur = min;

	scuboid_place = scuboid_block == -1 ?
		Inventory_SelectedBlock : (BlockID)scuboid_block;

	scuboid_active   = true;
	scuboid_lastTime = Stopwatch_Measure();
}

void SCuboidCommand_Tick(void) {
	cc_uint64 now;
	cc_uint64 delay_us;
	cc_uint32 delay_ms;

	if (!scuboid_active) return;

	delay_ms = SCUBOID_DEFAULT_DELAY_MS;

	delay_ms = Options_CuboidDelayMS ? Options_CuboidDelayMS : SCUBOID_DEFAULT_DELAY_MS;

	delay_us = (cc_uint64)delay_ms * 1000ULL;

	now = Stopwatch_Measure();
	if (Stopwatch_ElapsedMicroseconds(scuboid_lastTime, now) < delay_us) return;
	scuboid_lastTime = now;

	Game_ChangeBlock(scuboid_cur.x, scuboid_cur.y, scuboid_cur.z, scuboid_place);

	scuboid_cur.x++;
	if (scuboid_cur.x > scuboid_max.x) {
		scuboid_cur.x = scuboid_min.x;
		scuboid_cur.z++;
	}
	if (scuboid_cur.z > scuboid_max.z) {
		scuboid_cur.z = scuboid_min.z;
		scuboid_cur.y++;
	}
	if (scuboid_cur.y > scuboid_max.y) {
		scuboid_active = false;
		Chat_AddRaw("&aSCuboid complete.");
	}
}

static void SCuboidCommand_Execute(const cc_string* args, int argsCount) {
	cc_string value = *args;

	DrawOpCommand_ResetState();
	drawOp_name = "SCuboid";
	drawOp_Func = SCuboidCommand_Draw;

	DrawOpCommand_ExtractPersistArg(&value);
	scuboid_block = -1;

	if (value.length) {
		scuboid_block = DrawOpCommand_ParseBlock(&value);
		if (scuboid_block == -1) return;
	}

	DrawOpCommand_Begin();
}

static struct ChatCommand SCuboidCommand = {
	"SCuboid", SCuboidCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client scuboid [block] [persist]",
		"&eSlow cuboid: places blocks over time (no teleport).",
		"&eDelay is controlled by Cuboid delay option (ms).",
	}
};

/*########################################################################################################################*
*------------------------------------------------------DCuboidCommand----------------------------------------------------*
*#########################################################################################################################*/

static cc_bool dc_active;
static IVec3 dc_min, dc_max, dc_cur;
static BlockID dc_block;

static int dc_placed_in_burst;
static enum { DC_IDLE, DC_TP, DC_WAIT, DC_PLACE, DC_PAUSE } dc_state;

static cc_uint64 dc_lastTime;

#define DC_BURST_SIZE      12
#define DC_BURST_DELAY_US  990000
#define DC_TP_WAIT_US      9000

static void DC_TeleportToCurrent(void) {
    struct Entity* e = &Entities.CurPlayer->Base;
    struct LocationUpdate u;
    Vec3 p;

    p.x = dc_cur.x + 0.5f;
    p.y = dc_cur.y + 1.6f;
    p.z = dc_cur.z + 0.5f;

    u.flags = LU_HAS_POS;
    u.pos   = p;
    e->VTABLE->SetLocation(e, &u);
}

void DCuboidCommand_Tick(void) {
    cc_uint64 now = Stopwatch_Measure();

    if (!dc_active) return;

    switch (dc_state) {

    case DC_TP:
        DC_TeleportToCurrent();
        dc_lastTime = now;
        dc_state = DC_WAIT;
        break;

    case DC_WAIT:
        if (Stopwatch_ElapsedMicroseconds(dc_lastTime, now) >= DC_TP_WAIT_US) {
            dc_placed_in_burst = 0;
            dc_state = DC_PLACE;
        }
        break;

    case DC_PLACE:
        Game_ChangeBlock(dc_cur.x, dc_cur.y, dc_cur.z, dc_block);
        dc_placed_in_burst++;

        dc_cur.x++;
        if (dc_cur.x > dc_max.x) {
            dc_cur.x = dc_min.x;
            dc_cur.z++;
        }
        if (dc_cur.z > dc_max.z) {
            dc_cur.z = dc_min.z;
            dc_cur.y++;
        }

        if (dc_cur.y > dc_max.y) {
            dc_active = false;
            Chat_AddRaw("&aDCuboid complete.");
            return;
        }

        if (dc_placed_in_burst >= DC_BURST_SIZE) {
            dc_lastTime = now;
            dc_state = DC_PAUSE;
        }
        break;

    case DC_PAUSE:
        if (Stopwatch_ElapsedMicroseconds(dc_lastTime, now) >= DC_BURST_DELAY_US) {
            dc_state = DC_TP;
        }
        break;
    }
}

static void DCuboidCommand_Draw(IVec3 min, IVec3 max) {
    dc_min = min;
    dc_max = max;
    dc_cur = min;
    dc_block = Inventory_SelectedBlock;

    dc_active = true;
    dc_state  = DC_TP;
}

static void DCuboidCommand_Execute(const cc_string* args, int argsCount) {
    DrawOpCommand_ResetState();
    drawOp_name = "DCuboid";
    drawOp_Func = DCuboidCommand_Draw;
    DrawOpCommand_Begin();
}

static struct ChatCommand DCuboidCommand = {
    "DCuboid", DCuboidCommand_Execute,
    COMMAND_FLAG_SINGLEPLAYER_ONLY,
    {
        "&a/client dcuboid",
        "&eBurst cuboid with teleport sync",
        "&ePlaces blocks in safe bursts"
    }
};

/*########################################################################################################################*
*------------------------------------------------------LCuboidCommand----------------------------------------------------*
*#########################################################################################################################*/

static cc_bool lc_active;
static IVec3 lc_min, lc_max, lc_cur;
static BlockID lc_block;

static int lc_placed_in_burst;
static enum { LC_IDLE, LC_TP, LC_WAIT, LC_PLACE, LC_PAUSE } lc_state;

static cc_uint64 lc_lastTime;

#define LC_BURST_SIZE      1
#define LC_BURST_DELAY_US  34000
#define LC_TP_WAIT_US      19000

static void LC_TeleportToCurrent(void) {
    struct Entity* e = &Entities.CurPlayer->Base;
    struct LocationUpdate u;
    Vec3 p;

    p.x = lc_cur.x + 0.5f;
    p.y = lc_cur.y + 1.6f;
    p.z = lc_cur.z + 0.5f;

    u.flags = LU_HAS_POS;
    u.pos   = p;
    e->VTABLE->SetLocation(e, &u);
}

void LCuboidCommand_Tick(void) {
    cc_uint64 now = Stopwatch_Measure();

    if (!lc_active) return;

    switch (lc_state) {

    case LC_TP:
        LC_TeleportToCurrent();
        lc_lastTime = now;
        lc_state = LC_WAIT;
        break;

    case LC_WAIT:
        if (Stopwatch_ElapsedMicroseconds(lc_lastTime, now) >= LC_TP_WAIT_US) {
            lc_placed_in_burst = 0;
            lc_state = LC_PLACE;
        }
        break;

    case LC_PLACE:
        Game_ChangeBlock(lc_cur.x, lc_cur.y, lc_cur.z, lc_block);
        lc_placed_in_burst++;

        lc_cur.x++;
        if (lc_cur.x > lc_max.x) {
            lc_cur.x = lc_min.x;
            lc_cur.z++;
        }
        if (lc_cur.z > lc_max.z) {
            lc_cur.z = lc_min.z;
            lc_cur.y++;
        }

        if (lc_cur.y > lc_max.y) {
            lc_active = false;
            Chat_AddRaw("&aLCuboid complete.");
            return;
        }

        if (lc_placed_in_burst >= LC_BURST_SIZE) {
            lc_lastTime = now;
            lc_state = LC_PAUSE;
        }
        break;

    case LC_PAUSE:
        if (Stopwatch_ElapsedMicroseconds(lc_lastTime, now) >= LC_BURST_DELAY_US) {
            lc_state = LC_TP;
        }
        break;
    }
}

static void LCuboidCommand_Draw(IVec3 min, IVec3 max) {
    lc_min = min;
    lc_max = max;
    lc_cur = min;
    lc_block = Inventory_SelectedBlock;

    lc_active = true;
    lc_state  = LC_TP;
}

static void LCuboidCommand_Execute(const cc_string* args, int argsCount) {
    DrawOpCommand_ResetState();
    drawOp_name = "LCuboid";
    drawOp_Func = LCuboidCommand_Draw;
    DrawOpCommand_Begin();
}

static struct ChatCommand LCuboidCommand = {
    "LCuboid", LCuboidCommand_Execute,
    COMMAND_FLAG_SINGLEPLAYER_ONLY,
    {
        "&a/client lcuboid",
        "&eits slow and gentle, but no error like ''you cant build-'' blah blah",
        "&eits on test but atleast i tried."
    }
};

/*########################################################################################################################*
*------------------------------------------------------LolcatCommand-----------------------------------------------------*
*#########################################################################################################################*/

#include "Lolcat.h"

static void LolcatCommand_Execute(const cc_string* args, int argsCount) {
    Lolcat_Auto = !Lolcat_Auto;

    cc_string msg;
    char buf[64];
    String_InitArray(msg, buf);

    String_AppendConst(&msg, Lolcat_Auto ? "&aLolcat ON" : "&cLolcat OFF");
    Chat_Send(&msg, false);
}

static struct ChatCommand LolcatCommand = {
    "lolcat", LolcatCommand_Execute,
    COMMAND_FLAG_UNSPLIT_ARGS,
    {
        "&a/client lolcat <text>",
        "&a/client lolcat auto",
        "&eCursed lolcat translator",
    }
};

/*########################################################################################################################*
*----------------------------------------------------------uwuCommand------------------------------------------------------*
*#########################################################################################################################*/

static void UwUCommand_Execute(const cc_string* args, int argsCount) {
    cc_string out;
    char buf[256];
    String_InitArray(out, buf);

    if (argsCount > 0) {
        UwU_Transform(&args[0], &out);
        Chat_Send(&out, false);
    } else {
        UwU_Auto = !UwU_Auto;
        Chat_AddRaw(UwU_Auto ? "&aUwU auto ON" : "&cUwU auto OFF");
    }
}

static struct ChatCommand UwUCommand = {
    "uwu", UwUCommand_Execute,
    COMMAND_FLAG_UNSPLIT_ARGS,
    {
        "&a/client uwu <text>",
        "&eUwU-ifies your message or toggles auto mode",
    }
};

/*########################################################################################################################*
*------------------------------------------------------LeetCommand------------------------------------------------------*
*#########################################################################################################################*/

static char Leet_Map(char c) {
    switch (c) {
        case 'a': return '4';
        case 'A': return '4';
        case 'e': return '3';
        case 'E': return '3';
        case 'i': return '1';
        case 'I': return '1';
        case 'o': return '0';
        case 'O': return '0';
        case 's': return '5';
        case 'S': return '5';
        case 't': return '7';
        case 'T': return '7';
        case 'g': return '6';
        case 'G': return '6';
        default:  return c;
    }
}

static void Leet_Transform(const cc_string* src, cc_string* dst) {
    dst->length = 0;

    for (int i = 0; i < src->length; i++) {
        char c = src->buffer[i];
        String_Append(dst, Leet_Map(c));
    }
}

static void LeetCommand_Execute(const cc_string* args, int argsCount) {
    cc_string out;
    char buf[STRING_SIZE];

    String_InitArray(out, buf);

    if (argsCount > 0) {
        Leet_Transform(&args[0], &out);
    } else {
        String_AppendConst(&out, "7YP3 50M37H1N6");
    }

    Chat_Send(&out, false);
}

static struct ChatCommand LeetCommand = {
    "leet", LeetCommand_Execute,
    COMMAND_FLAG_UNSPLIT_ARGS,
    {
        "&a/client leet <text>",
        "&eTurns your message into 1337 speak",
    }
};

/*########################################################################################################################*
*-------------------------------------------------------ReplaceCommand-----------------------------------------------------*
*#########################################################################################################################*/
static int replace_source, replace_target;

static void ReplaceCommand_Draw(IVec3 min, IVec3 max) {
	BlockID cur, source, toPlace;
	int x, y, z;

	source  = (BlockID)replace_source;
	toPlace = (BlockID)replace_target;
	if (replace_target == -1) toPlace = Inventory_SelectedBlock;

	for (y = min.y; y <= max.y; y++) {
		for (z = min.z; z <= max.z; z++) {
			for (x = min.x; x <= max.x; x++) {
				cur = World_GetBlock(x, y, z);
				if (cur != source) continue;
				Game_ChangeBlock(x, y, z, toPlace);
			}
		}
	}
}

static void ReplaceCommand_Execute(const cc_string* args, int argsCount) {
	cc_string value = *args;
	cc_string parts[2];
	int count;

	DrawOpCommand_ResetState();
	drawOp_name = "Replace";
	drawOp_Func = ReplaceCommand_Draw;

	DrawOpCommand_ExtractPersistArg(&value);
	replace_target = -1;

	if (!value.length) {
		Chat_AddRaw("&eReplace: &cAt least one argument is required"); return;
	}
	count = String_UNSAFE_Split(&value, ' ', parts, 2);

	replace_source = DrawOpCommand_ParseBlock(&parts[0]);
	if (replace_source == -1) return;

	if (count > 1) {
		replace_target = DrawOpCommand_ParseBlock(&parts[1]);
		if (replace_target == -1) return;
	}

	DrawOpCommand_Begin();
}

static struct ChatCommand ReplaceCommand = {
	"Replace", ReplaceCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client replace [source] [replacement] [persist]",
		"&eReplaces all [source] blocks between two points with [replacement].",
		"&eIf no [replacement] is given, replaces with your currently held block.",
		"&e  If persist is given and is \"yes\", then the command",
		"&e  will repeatedly replace, without needing to be typed in again.",
	}
};


/*########################################################################################################################*
*------------------------------------------------------TeleportCommand----------------------------------------------------*
*#########################################################################################################################*/

static void TeleportCommand_Execute(const cc_string* args, int argsCount) {
    struct Entity* e = &Entities.CurPlayer->Base;
    struct LocationUpdate update;
    Vec3 target, cur;
    int i, steps = 20;

    if (argsCount != 3) {
        Chat_AddRaw("&e/client tp: &cYou didn't specify X Y Z");
        return;
    }

    if (!Convert_ParseFloat(&args[0], &target.x) ||
        !Convert_ParseFloat(&args[1], &target.y) ||
        !Convert_ParseFloat(&args[2], &target.z)) {
        Chat_AddRaw("&e/client tp: &cCoordinates must be decimals");
        return;
    }

    cur = e->Position;

    for (i = 1; i <= steps; i++) {
        Vec3 step;
        step.x = cur.x + (target.x - cur.x) * i / steps;
        step.y = cur.y + (target.y - cur.y) * i / steps;
        step.z = cur.z + (target.z - cur.z) * i / steps;

        update.flags = LU_HAS_POS;
        update.pos   = step;
        e->VTABLE->SetLocation(e, &update);
    }
}

static struct ChatCommand TeleportCommand = {
	"TP", TeleportCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY,
	{
		"&a/client tp [x y z]",
		"&eMoves you to the given coordinates.",
	}
};

/*########################################################################################################################*
*------------------------------------------------------RenameCommand----------------------------------------------------*
*#########################################################################################################################*/

void RenameCMD_Execute(const cc_string* args, int argsCount)
{
	static const cc_string title = String_FromConst("&fClientName changed!!!!!!");
	static const cc_string reason = String_FromConst("&fPlease rejoin using the reconnect button - Hackmee");
	Server.AppName = String_FromReadonly(args->buffer);
	Game_Disconnect(&title, &reason);
	
}

static struct ChatCommand RenameCommand = {
	"rename", RenameCMD_Execute,
	0,
	{
		"&a/client rename [argument]",
		"&eRenames the client for the server. credits from Hackmee repo",
	}
};

/*########################################################################################################################*
*-------------------------------------------------------ChatMacrosCommand------------------------------------------------*
*#########################################################################################################################*/
#include "ChatMacros.h"

static void MacroCommand_Execute(const cc_string* args, int argsCount) {
    if (argsCount < 2) {
        Chat_AddRaw("&ebozo its broken, i gave up");
        return;
    }

    ChatMacros_Add(&args[0], &args[1]);
    Chat_AddRaw("&aMacro added.");
}

static struct ChatCommand MacroCommand = {
    "macro", MacroCommand_Execute,
    COMMAND_FLAG_UNSPLIT_ARGS,
    {
        "&a/client macro <trigger> <text>",
        "&eAdds a chat macro(abandoned)"
    }
};

/*########################################################################################################################*
*----------------------------------------------------------NtpCommand----------------------------------------------------*
*#########################################################################################################################*/

static char ToLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static cc_bool String_ContainsCaseless(const cc_string* haystack, const cc_string* needle) {
    int i, j;
    if (!needle->length || needle->length > haystack->length) return false;

    for (i = 0; i <= haystack->length - needle->length; i++) {
        for (j = 0; j < needle->length; j++) {
            char a = ToLowerChar(haystack->buffer[i + j]);
            char b = ToLowerChar(needle->buffer[j]);
            if (a != b) break;
        }
        if (j == needle->length) return true;
    }
    return false;
}


static struct Entity* FindPlayerByName(
    const cc_string* name,
    int* matchCount
) {
    struct Entity* result = NULL;
    *matchCount = 0;

    for (int i = 0; i < TABLIST_MAX_NAMES; i++) {
        if (!TabList.NameOffsets[i]) continue;

        cc_string n = TabList_UNSAFE_GetPlayer(i);

        if (String_CaselessEquals(&n, name)) {
            *matchCount = 1;
            return &NetPlayers_List[i].Base;
        }

        if (String_ContainsCaseless(&n, name)) {
            (*matchCount)++;
            result = &NetPlayers_List[i].Base;
        }
    }

    return result;
}


static void TeleportToEntity(struct Entity* target) {
    struct Entity* me = &Entities.CurPlayer->Base;

    Vec3 pos = target->Position;
    pos.y += 0.0f; 

    struct LocationUpdate u;
    u.flags = LU_HAS_POS | LU_POS_ABSOLUTE_INSTANT;
    u.pos   = pos;

    me->VTABLE->SetLocation(me, &u);
}

static void NtpCommand_Execute(const cc_string* args, int argsCount) {
    if (argsCount == 0) {
        Chat_AddRaw("&e/client ntp <player>");
        return;
    }

    int matches = 0;
    struct Entity* target = FindPlayerByName(&args[0], &matches);

    if (matches == 0) {
        Chat_AddRaw("&cPlayer not found.");
        return;
    }

    if (matches > 1) {
        Chat_AddRaw("&eMultiple players match:");

        for (int i = 0; i < MAX_NET_PLAYERS; i++) {
            struct NetPlayer* p = &NetPlayers_List[i];
            if (!p->Base.Flags) continue;

            cc_string n = TabList_UNSAFE_GetPlayer(i);
            if (!n.length) continue;

            if (String_ContainsCaseless(&n, &args[0])) {
                Chat_AddRaw("&7 - ");
                Chat_Add(&n);
            }
        }

        Chat_AddRaw("&eBe more specific. ;-;");
        return;
    }

    TeleportToEntity(target);
    Chat_AddRaw("&aTeleported.");
}

static struct ChatCommand NtpCommand = {
    "ntp", NtpCommand_Execute,
    0,
    {
        "&a/client ntp <player>",
        "&eTeleport to another player (client-side)"
    }
};

/*########################################################################################################################*
*------------------------------------------------------ClientHaxCommand--------------------------------------------------*
*#########################################################################################################################*/

static void HaxCommand_Execute(const cc_string* args, int argsCount) {
    if (argsCount == 0) {
        Chat_AddRaw(ClientHax_Enabled
            ? "&aClient Hax: ON"
            : "&cClient Hax: OFF");
        return;
    }

    if (String_CaselessEqualsConst(&args[0], "on")) {
        ClientHax_Enabled = true;
        Chat_AddRaw("&aClient hacks enabled");
    }
    else if (String_CaselessEqualsConst(&args[0], "off")) {
        ClientHax_Enabled = false;
        Chat_AddRaw("&cClient hacks disabled");
    }
}


static struct ChatCommand HaxCommand = {
    "hax", HaxCommand_Execute,
    0,
    {
        "&a/client hax on",
        "&a/client hax off",
        "&eToggle all client-side hacks",
        "u can do it all the time btw,like leavin or rejoinin."
    }
};

/*########################################################################################################################*
*-----------------------------------------------------FullBrightCommand--------------------------------------------------*
*#########################################################################################################################*/

static cc_bool fullbright_active;

static PackedCol fb_oldSky;
static PackedCol fb_oldFog;
static PackedCol fb_oldShadow;
static PackedCol fb_oldSun;

static void FullBright_Enable(void) {
    fb_oldSky    = Env.SkyCol;
    fb_oldFog    = Env.FogCol;
    fb_oldShadow = Env.ShadowCol;
    fb_oldSun    = Env.SunCol;

    Env_SetSkyCol(PACKEDCOL_WHITE);
    Env_SetFogCol(PACKEDCOL_WHITE);
    Env_SetShadowCol(PACKEDCOL_WHITE);
    Env_SetSunCol(PACKEDCOL_WHITE);

    fullbright_active = true;
    Chat_AddRaw("&aFullbright enabled");
}

static void FullBright_Disable(void) {
    Env_SetSkyCol(fb_oldSky);
    Env_SetFogCol(fb_oldFog);
    Env_SetShadowCol(fb_oldShadow);
    Env_SetSunCol(fb_oldSun);

    fullbright_active = false;
    Chat_AddRaw("&cFullbright disabled");
}

static void FullBrightCommand_Execute(const cc_string* args, int argsCount) {
    if (!fullbright_active) {
        FullBright_Enable();
    } else {
        FullBright_Disable();
    }
}

static struct ChatCommand FullBrightCommand = {
    "fullbright", FullBrightCommand_Execute,
    0,
    {
        "&a/client fullbright",
        "&eToggles fullbright lighting"
    }
};


/*########################################################################################################################*
*------------------------------------------------------BadAppleCommand--------------------------------------------------*
*#########################################################################################################################*/

#include "BadAppleFrames.h"

static int badapple_index;
static cc_bool badapple_active;

static void BadApple_SendFrame(void) {
    if (!badapple_active) return;

    if (badapple_index >= BADAPPLE_FRAME_COUNT) {
        badapple_active = false;
        Chat_AddRaw("&aBad Apple finished.");
        return;
    }

    for (int i = 0; i < BADAPPLE_FRAME_HEIGHT; i++) {
        const char* line = BADAPPLE_FRAMES[badapple_index][i];

        cc_string msg = String_FromRaw((char*)line, String_Length(line));
        Chat_Send(&msg, false);
    }

    badapple_index++;
}

static void BadAppleCommand_Execute(const cc_string* args, int argsCount) {
    if (argsCount >= 1 && String_CaselessEqualsConst(&args[0], "stop")) {
        if (badapple_active) {
            badapple_active = false;
            Chat_AddRaw("&cBad Apple stopped.");
        } else {
            Chat_AddRaw("&eBad Apple is not running.");
        }
        return;
    }

    badapple_index  = 0;
    badapple_active = true;

    Chat_AddRaw("&dBad Apple started...");
    BadApple_SendFrame();
}

void BadApple_Tick(void) {
    static cc_uint64 last;
    cc_uint64 now = Stopwatch_Measure();

    if (Stopwatch_ElapsedMicroseconds(last, now) < 70000) return;
    last = now;

    BadApple_SendFrame();
}

static struct ChatCommand BadAppleCommand = {
    "badapple", BadAppleCommand_Execute,
    0,
    {
        "&a/client badapple [&fstop&a]",
        "&ePlays Bad Apple in chat",
    }
};

/*########################################################################################################################*
*------------------------------------------------------StreamCommand----------------------------------------------------*
*#########################################################################################################################*/

#include "Platform.h"
#include "String.h"
#include "Chat.h"

static cc_socket stream_sock;
static cc_bool   stream_active;

static char      stream_raw[512];
static cc_string stream_buffer;

static cc_uint64 stream_last_send;
#define STREAM_CHAT_DELAY_US 80000

static void Stream_Disconnect(void) {
    if (!stream_active) return;

    Socket_Close(stream_sock);
    stream_active = false;
    stream_buffer.length = 0;

    Chat_AddRaw("&7[Stream] disconnected");
}

static cc_bool Stream_Connect(int port) {
    cc_sockaddr addrs[4];
    int count = 0;

    cc_string ip = String_FromConst("127.0.0.1");

    if (Socket_ParseAddress(&ip, port, addrs, &count) != 0 || count <= 0)
        return false;

    if (Socket_Create(&stream_sock, &addrs[0], false) != 0)
        return false;

    if (Socket_Connect(stream_sock, &addrs[0]) != 0) {
        Socket_Close(stream_sock);
        return false;
    }

    String_InitArray(stream_buffer, stream_raw);
    stream_buffer.length = 0;

    stream_active = true;
    return true;
}

static void Stream_SendChatLine(const char* text, int len) {
    cc_string msg;

    msg = String_FromRaw((char*)text, len);
    Chat_Send(&msg, false);
}

void Stream_Tick(void) {
    cc_uint64 now = Stopwatch_Measure();
    if (Stopwatch_ElapsedMicroseconds(stream_last_send, now) < STREAM_CHAT_DELAY_US)
        return;

    stream_last_send = now;

    if (!stream_active) return;

    cc_uint8 buf[256];
    cc_uint32 read;

    if (Socket_Read(stream_sock, buf, sizeof(buf), &read) != 0)
        return;

    if (!read) return;

    for (cc_uint32 i = 0; i < read; i++) {
        char c = (char)buf[i];

        if (c == '\n') {
            if (stream_buffer.length > 0) {
                Chat_Send(&stream_buffer, false);
                stream_buffer.length = 0;
            }
        } else {
            if (stream_buffer.length < stream_buffer.capacity - 1) {
                stream_buffer.buffer[stream_buffer.length++] = c;
            }
        }
    }
}

static void StreamCommand_Execute(const cc_string* args, int argsCount) {
    if (argsCount > 0 && String_CaselessEqualsConst(args, "stop")) {
        Stream_Disconnect();
        return;
    }

    if (stream_active) {
        Chat_AddRaw("&c[Stream] already running");
        return;
    }

    if (!Stream_Connect(45678)) {
        Chat_AddRaw("&c[Stream] failed to connect (is python running?)");
        return;
    }

    Chat_AddRaw("&a[Stream] connected");
}

static struct ChatCommand StreamCommand = {
    "stream", StreamCommand_Execute,
    0,
    {
        "&a/client stream [&fport&a] [&fstop&a]",
        "&eReads newline-delimited text from 127.0.0.1:<port> and sends to public chat.",
        "&eIf python sends !FRAME/!END, it will treat the block as a frame."
    }
};

/*########################################################################################################################*
*------------------------------------------------------BStreamCommand----------------------------------------------------*
*#########################################################################################################################*/

static void BStreamCommand_Execute(const cc_string* args, int argsCount) {
    int w, h, x, y, z, port;

    if (argsCount > 0 && String_CaselessEqualsConst(&args[0], "stop")) {
        BStream_Stop();
        return;
    }

    if (argsCount < 6 || !String_CaselessEqualsConst(&args[0], "start")) {
        Chat_AddRaw("&eUsage: &f/client bstream start <w> <h> <x> <y> <z> [port]");
        Chat_AddRaw("&eExample: &f/client bstream start 60 45 0 60 0");
        return;
    }

    port = 45679;
    Convert_ParseInt(&args[1], &w);
    Convert_ParseInt(&args[2], &h);
    Convert_ParseInt(&args[3], &x);
    Convert_ParseInt(&args[4], &y);
    Convert_ParseInt(&args[5], &z);
    if (argsCount > 6) Convert_ParseInt(&args[6], &port);

    if (!BStream_Start(w, h, x, y, z, port)) {
        Chat_AddRaw("&c[BStream] failed to start. Is python running? (or bad bounds)");
        return;
    }

    Chat_AddRaw("&a[BStream] connected. Painting blocks...");
}

static struct ChatCommand BStreamCommand = {
    "bstream", BStreamCommand_Execute,
    0,
    {
        "&a/client bstream start <w> <h> <x> <y> <z> [port]",
        "&a/client bstream stop",
        "&eStreams frames over TCP and draws them as blocks on the X/Z plane.",
        "&eSender should send exactly <h> lines per frame, newline-delimited."
    }
};

/*########################################################################################################################*
*------------------------------------------------------GeminiCommand----------------------------------------------------*
*#########################################################################################################################*/

extern cc_bool Gemini_Send(const cc_string* msg);

static void GeminiCommand_Execute(const cc_string* args, int argsCount) {
    if (!argsCount) {
        Chat_AddRaw("&eUsage: &f/client gemini <message>");
        return;
    }

    cc_string msg; char buf[512];
    String_InitArray(msg, buf);

    for (int i = 0; i < argsCount; i++) {
        if (i) String_Append(&msg, ' ');
        String_AppendString(&msg, &args[i]);
    }

    if (!Gemini_Send(&msg))
        Chat_AddRaw("&c[Gemini] Python server not running.");
}

static struct ChatCommand GeminiCommand = {
    "gemini", GeminiCommand_Execute,
    COMMAND_FLAG_UNSPLIT_ARGS,
    {
        "&a/client gemini <message>",
        "&eTalk to Gemini via local python server"
    }
};

/*########################################################################################################################*
*------------------------------------------------------AntiAFKCommand----------------------------------------------------*
*#########################################################################################################################*/

static cc_bool AntiAFK_Enabled;
static float   AntiAFK_Dir = 1.0f;

static void AntiAFK_Tick(struct ScheduledTask* task) {
    if (!AntiAFK_Enabled) return;

    struct Entity* e = &Entities.CurPlayer->Base;
    struct LocationUpdate u;

    u.flags = LU_HAS_YAW;
    u.yaw   = e->Yaw + (1.5f * AntiAFK_Dir);

    e->VTABLE->SetLocation(e, &u);

    AntiAFK_Dir = -AntiAFK_Dir;
}


static void AntiAFKCommand_Execute(const cc_string* args, int argsCount) {
    if (!AntiAFK_Enabled) {
        AntiAFK_Enabled = true;
        ScheduledTask_Add(2.0f, AntiAFK_Tick);
        Chat_AddRaw("&aAnti-AFK enabled");
    } else {
        AntiAFK_Enabled = false;
        Chat_AddRaw("&cAnti-AFK disabled");
    }
}

static struct ChatCommand AntiAFKCommand = {
    "antiafk", AntiAFKCommand_Execute,
    0,
    {
        "&a/client antiafk",
        "&ePrevents AFK kick by gently moving your view"
    }
};

/*########################################################################################################################*
*-----------------------------------------------------AutoClickerCommand-------------------------------------------------*
*#########################################################################################################################*/

extern cc_bool autoclick_left;
extern cc_bool autoclick_right;
extern float autoclick_delay;

static void AutoClickCommand_Execute(const cc_string* args, int argsCount) {
    if (argsCount < 1) {
        Chat_AddRaw("&e/client autoclick left|right|off [delay] like 0.1 or 0.05 fast shit or 5 slow ass and... idk");
        return;
    }

    if (String_CaselessEqualsConst(&args[0], "off")) {
        autoclick_left  = false;
        autoclick_right = false;
        Chat_AddRaw("&cAutoClick disabled");
        return;
    }

    if (argsCount >= 2) {
        Convert_ParseFloat(&args[1], &autoclick_delay);
        if (autoclick_delay < 0.01f) autoclick_delay = 0.01f;
        if (autoclick_delay > 5.0f)  autoclick_delay = 5.0f;
    }

    if (String_CaselessEqualsConst(&args[0], "left")) {
        autoclick_left = true;
        autoclick_right = false;
    }
    else if (String_CaselessEqualsConst(&args[0], "right")) {
        autoclick_right = true;
        autoclick_left = false;
    }

    cc_string msg; char buf[64];
    String_InitArray(msg, buf);
    String_AppendConst(&msg, "&aAutoClick ON (");
    String_AppendFloat(&msg, autoclick_delay, 2);
    String_AppendConst(&msg, "s)");
    Chat_Add(&msg);
}

static struct ChatCommand AutoClickCommand = {
    "autoclick", AutoClickCommand_Execute,
    0,
    {
        "&a/client autoclick left",
        "&a/client autoclick right",
        "&a/client autoclick off"
    }
};

/*########################################################################################################################*
*------------------------------------------------------chatgptCommand----------------------------------------------------*
*#########################################################################################################################*/

extern cc_bool ChatGPT_Send(const cc_string* msg);

static void GPTCommand_Execute(const cc_string* args, int argsCount) {
    if (argsCount == 0) {
        Chat_AddRaw("&eUsage: &f/client gpt <message>");
        return;
    }

    cc_string msg;
    char buf[512];
    String_InitArray(msg, buf);

    for (int i = 0; i < argsCount; i++) {
        if (i) String_Append(&msg, ' ');
        String_AppendString(&msg, &args[i]);
    }

    {
        cc_string out;
        char tmp[600];
        String_InitArray(out, tmp);
        String_AppendConst(&out, "&7asked ChatGPT(API) &f");
        String_AppendString(&out, &msg);
        Chat_Send(&out, false);
    }

    if (!ChatGPT_Send(&msg)) {
        Chat_AddRaw("&c[GPT] Python server not running");
    }
}

static struct ChatCommand GPTCommand = {
    "gpt", GPTCommand_Execute,
    COMMAND_FLAG_UNSPLIT_ARGS,
    {
        "&a/client gpt <message>",
        "&eTalk to ChatGPT"
    }
};

/*########################################################################################################################*
*------------------------------------------------------PosFlyCommand----------------------------------------------------*
*#########################################################################################################################*/

static cc_bool posfly_active;

static int PosFly_GetDir(float yaw) {
    int dir = (int)(yaw / 90.0f + 0.5f);
    if (dir < 0) dir += 4;
    return dir & 3;
}

static void PosFly_Tick(struct ScheduledTask* task) {
    if (!posfly_active) {
        task->interval = 0;
        return;
    }

    struct Entity* e = &Entities.CurPlayer->Base;
    Vec3 pos = e->Position;

    int dir = PosFly_GetDir(e->Yaw);
    float step = 0.06f;

    if (dir == 0) pos.z -= step;
    if (dir == 1) pos.x += step;
    if (dir == 2) pos.z += step;
    if (dir == 3) pos.x -= step;

    struct LocationUpdate u;
    u.flags = LU_HAS_POS;
    u.pos   = pos;
    e->VTABLE->SetLocation(e, &u);
}

static void PosFlyCommand_Execute(const cc_string* args, int argsCount) {
    if (!posfly_active) {
        posfly_active = true;
        ScheduledTask_Add(0.0125, PosFly_Tick);
        Chat_AddRaw("&aPosFly enabled");
    } else {
        posfly_active = false;
        Chat_AddRaw("&cPosFly disabled");
    }
}

static struct ChatCommand PosFlyCommand = {
    "posfly", PosFlyCommand_Execute,
    0,
    {
        "&a/client posfly",
        "&eSmooth yaw-based flying",
    }
};

/*########################################################################################################################*
*------------------------------------------------------RickRollCommand----------------------------------------------------*
*#########################################################################################################################*/

static cc_bool   rick_active;
static int       rick_index;
static cc_uint64 rick_last;

/* Short iconic fragments only (copyright-safe) */
static const char* rick_lines[] = {
    "Never gonna give you up",
    "Never gonna let you down",
    "Never gonna run around and desert you",
    "Never gonna make you cry",
    "Never gonna say goodbye",
    "Never gonna tell a lie... and hurt you"
};

static void RickRoll_Tick(struct ScheduledTask* task) {
    cc_uint64 now;

    if (!rick_active) return;

    now = Stopwatch_Measure();
    if (Stopwatch_ElapsedMicroseconds(rick_last, now) < 1400000) return;
    rick_last = now;

    cc_string msg = String_FromReadonly(rick_lines[rick_index]);
    Chat_Send(&msg, false);

    rick_index++;
    if (rick_index >= Array_Elems(rick_lines)) {
        rick_active = false;
        Chat_AddRaw("&aRickroll finished.");
    }
}

static void RickRollCommand_Execute(const cc_string* args, int argsCount) {
    if (argsCount > 0 && String_CaselessEqualsConst(&args[0], "stop")) {
        rick_active = false;
        return;
    }

    if (rick_active) {
        Chat_AddRaw("&eRickroll already running.");
        return;
    }

    rick_active = true;
    rick_index  = 0;
    rick_last   = Stopwatch_Measure();

    ScheduledTask_Add(0.05, RickRoll_Tick);
    Chat_AddRaw("&dBET...");
}

static struct ChatCommand RickRollCommand = {
    "rickroll", RickRollCommand_Execute,
    0,
    {
        "&a/client rickroll [&fstop&a]",
        "&eRickrolls chat (server-visible)",
    }
};

/*########################################################################################################################*
*--------------------------------------------------------N8BallCommand--------------------------------------------------*
*#########################################################################################################################*/

static cc_bool   n8_active;
static int       n8_index;
static cc_uint64 n8_last;

static const char* n8_answers[] = {
    "hell nah",
    "nah vro",
    "maybe???",
    "lowkey yes",
    "ABSOLUTELY NOT",
    "bro what",
    "yeah sure",
    "ask again later",
    "idk man",
    "sounds cursed",
	"mehhhhhhhhhhh",
	"vro im NOT google gng",
	"absolute peak massage",
	"vros typing shit :sob::pray:"
};

#define N8BALL_COUNT (sizeof(n8_answers) / sizeof(n8_answers[0]))

static void N8Ball_Tick(void) {
    cc_uint64 now = Stopwatch_Measure();

    if (!n8_active) return;
    if (Stopwatch_ElapsedMicroseconds(n8_last, now) < 900000) return;

    n8_active = false;

    cc_string msg;
    char buf[128];

    String_InitArray(msg, buf);
    String_AppendConst(&msg, "&b[&7N&b8Ball]:&f ");
    String_AppendConst(&msg, n8_answers[n8_index]);

    Chat_Send(&msg, false);
}

static void N8BallCommand_Execute(const cc_string* args, int argsCount) {
    cc_string msg;
    char buf[256];

    String_InitArray(msg, buf);
    String_AppendConst(&msg, "&easked the %7N%b8Ball: ");

    if (argsCount > 0) {
        String_AppendString(&msg, &args[0]);
    } else {
        String_AppendConst(&msg, "...");
    }

    Chat_Send(&msg, false);

    n8_index = (int)(Stopwatch_Measure() % N8BALL_COUNT);
    n8_last  = Stopwatch_Measure();
    n8_active = true;
}

void N8Ball_TickHook(void) {
    N8Ball_Tick();
}

static struct ChatCommand N8BallCommand = {
    "n8ball", N8BallCommand_Execute,
    COMMAND_FLAG_UNSPLIT_ARGS,
    {
        "&a/client n8ball <question>",
        "&eTotally accurate future prediction device",
    }
};

/*########################################################################################################################*
*------------------------------------------------------NoSetBack-----------------------------------------------------------*
*#########################################################################################################################*/

/*credits from velocity btw... AHHHHHHHHHHHHHHHHHHHHHHH-*/

cc_bool NoSetBack_enabled = false;

static void NoSetBackCommand_Execute(const cc_string* args, int argsCount) {
    NoSetBack_enabled = !NoSetBack_enabled;
    Chat_AddRaw(NoSetBack_enabled ? "&aNoSetBack enabled" : "&cNoSetBack disabled");
}

static struct ChatCommand NoSetBackCommand = {
    "NoSetBack", NoSetBackCommand_Execute, 0,
    {
        "&a/client NoSetBack.",
        "&ePrevents the server from teleporting you. - credits from velocity",
    }
};


/*########################################################################################################################*
*------------------------------------------------------BlockEditCommand----------------------------------------------------*
*#########################################################################################################################*/
static cc_bool BlockEditCommand_GetInt(const cc_string* str, const char* name, int* value, int min, int max) {
	if (!Convert_ParseInt(str, value)) {
		Chat_Add1("&eBlockEdit: &e%c must be an integer", name);
		return false;
	}

	if (*value < min || *value > max) {
		Chat_Add3("&eBlockEdit: &e%c must be between %i and %i", name, &min, &max);
		return false;
	}
	return true;
}

static cc_bool BlockEditCommand_GetTexture(const cc_string* str, int* tex) {
	return BlockEditCommand_GetInt(str, "Texture", tex, 0, ATLAS1D_MAX_ATLASES - 1);
}

static cc_bool BlockEditCommand_GetCoords(const cc_string* str, Vec3* coords) {
	cc_string parts[3];
	IVec3 xyz;

	int numParts = String_UNSAFE_Split(str, ' ', parts, 3);
	if (numParts != 3) {
		Chat_AddRaw("&eBlockEdit: &c3 values are required for a coordinate (X Y Z)");
		return false;
	}

	if (!BlockEditCommand_GetInt(&parts[0], "X coord", &xyz.x, -127, 127)) return false;
	if (!BlockEditCommand_GetInt(&parts[1], "Y coord", &xyz.y, -127, 127)) return false;
	if (!BlockEditCommand_GetInt(&parts[2], "Z coord", &xyz.z, -127, 127)) return false;

	coords->x = xyz.x / 16.0f;
	coords->y = xyz.y / 16.0f;
	coords->z = xyz.z / 16.0f;
	return true;
}

static cc_bool BlockEditCommand_GetBool(const cc_string* str, const char* name, cc_bool* value) {
	if (String_CaselessEqualsConst(str, "true") || String_CaselessEqualsConst(str, "yes")) {
		*value = true;
		return true;
	}

	if (String_CaselessEqualsConst(str, "false") || String_CaselessEqualsConst(str, "no")) {
		*value = false;
		return true;
	}

	Chat_Add1("&eBlockEdit: &e%c must be either &ayes &eor &ano", name);
	return false;
}


static void BlockEditCommand_Execute(const cc_string* args, int argsCount__) {
	cc_string parts[3];
	cc_string* prop;
	cc_string* value;
	int argsCount, block, v;
	cc_bool b;
	Vec3 coords;

	if (String_CaselessEqualsConst(args, "properties")) {
		Chat_AddRaw("&eEditable block properties:");
		Chat_AddRaw("&a  name &e- Sets the name of the block");
		Chat_AddRaw("&a  all &e- Sets textures on all six sides of the block");
		Chat_AddRaw("&a  sides &e- Sets textures on four sides of the block");
		Chat_AddRaw("&a  left/right/front/back/top/bottom &e- Sets one texture");
		Chat_AddRaw("&a  collide &e- Sets collision mode of the block");
		Chat_AddRaw("&a  drawmode &e- Sets draw mode of the block");
		Chat_AddRaw("&a  min/max &e- Sets min/max corner coordinates of the block");
		Chat_AddRaw("&eSee &a/client blockedit properties 2 &efor more properties");
		return;
	}
	if (String_CaselessEqualsConst(args, "properties 2")) {
		Chat_AddRaw("&eEditable block properties (page 2):");
		Chat_AddRaw("&a  walksound &e- Sets walk/step sound of the block");
		Chat_AddRaw("&a  breaksound &e- Sets break sound of the block");
		Chat_AddRaw("&a  fullbright &e- Sets whether the block is fully lit");
		Chat_AddRaw("&a  blockslight &e- Sets whether the block stops light");
		return;
	}

	argsCount = String_UNSAFE_Split(args, ' ', parts, 3);
	if (argsCount < 3) {
		Chat_AddRaw("&eBlockEdit: &eThree arguments required &e(See &a/client help blockedit&e)");
		return;
	}

	block = Block_Parse(&parts[0]);
	if (block == -1) {
		Chat_Add1("&eBlockEdit: &c\"%s\" is not a valid block name or ID", &parts[0]);
		return;
	}

	/* TODO: Redo as an array */
	prop  = &parts[1];
	value = &parts[2];
	if (String_CaselessEqualsConst(prop, "name")) {
		Block_SetName(block, value);
	} else if (String_CaselessEqualsConst(prop, "all")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_SetSide(v, block);
		Block_Tex(block, FACE_YMAX) = v;
		Block_Tex(block, FACE_YMIN) = v;
	} else if (String_CaselessEqualsConst(prop, "sides")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_SetSide(v, block);
	} else if (String_CaselessEqualsConst(prop, "left")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_XMIN) = v;
	} else if (String_CaselessEqualsConst(prop, "right")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_XMAX) = v;
	} else if (String_CaselessEqualsConst(prop, "bottom")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_YMIN) = v;
	}  else if (String_CaselessEqualsConst(prop, "top")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_YMAX) = v;
	} else if (String_CaselessEqualsConst(prop, "front")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_ZMIN) = v;
	} else if (String_CaselessEqualsConst(prop, "back")) {
		if (!BlockEditCommand_GetTexture(value, &v)) return;

		Block_Tex(block, FACE_ZMAX) = v;
	} else if (String_CaselessEqualsConst(prop, "collide")) {
		if (!BlockEditCommand_GetInt(value, "Collide mode", &v, 0, COLLIDE_CLIMB)) return;

		Blocks.Collide[block] = v;
	} else if (String_CaselessEqualsConst(prop, "drawmode")) {
		if (!BlockEditCommand_GetInt(value, "Draw mode", &v, 0, DRAW_SPRITE)) return;

		Blocks.Draw[block] = v;
	} else if (String_CaselessEqualsConst(prop, "min")) {
		if (!BlockEditCommand_GetCoords(value, &coords)) return;

		Blocks.MinBB[block] = coords;
	} else if (String_CaselessEqualsConst(prop, "max")) {
		if (!BlockEditCommand_GetCoords(value, &coords)) return;

		Blocks.MaxBB[block] = coords;
	} else if (String_CaselessEqualsConst(prop, "walksound")) {
		if (!BlockEditCommand_GetInt(value, "Sound", &v, 0, SOUND_COUNT - 1)) return;

		Blocks.StepSounds[block] = v;
	} else if (String_CaselessEqualsConst(prop, "breaksound")) {
		if (!BlockEditCommand_GetInt(value, "Sound", &v, 0, SOUND_COUNT - 1)) return;

		Blocks.DigSounds[block]  = v;
	} else if (String_CaselessEqualsConst(prop, "fullbright")) {
		if (!BlockEditCommand_GetBool(value, "Full brightness", &b))  return;
		//TODO: Fix this, brightness isn't just a bool anymore
		Blocks.Brightness[block] = b;
	} else if (String_CaselessEqualsConst(prop, "blockslight")) {
		if (!BlockEditCommand_GetBool(value, "Blocks light", &b)) return;

		Blocks.BlocksLight[block] = b;
	} else {
		Chat_Add1("&eBlockEdit: &eUnknown property %s &e(See &a/client help blockedit&e)", prop);
		return;
	}

	Block_DefineCustom(block, false);
}

static struct ChatCommand BlockEditCommand = {
	"BlockEdit", BlockEditCommand_Execute,
	COMMAND_FLAG_SINGLEPLAYER_ONLY | COMMAND_FLAG_UNSPLIT_ARGS,
	{
		"&a/client blockedit [block] [property] [value]",
		"&eEdits the given property of the given block",
		"&a/client blockedit properties",
		"&eLists the editable block properties",
	}
};


/*########################################################################################################################*
*------------------------------------------------------Commands component-------------------------------------------------*
*#########################################################################################################################*/

static void OnInit(void) {
	Commands_Register(&GpuInfoCommand);
	Commands_Register(&HelpCommand);
	Commands_Register(&RenderTypeCommand);
	Commands_Register(&ResolutionCommand);
	Commands_Register(&ModelCommand);
	Commands_Register(&SkinCommand);
	Commands_Register(&TeleportCommand);
	Commands_Register(&ClearDeniedCommand);
	Commands_Register(&MotdCommand);
	Commands_Register(&PlaceCommand);
	Commands_Register(&BlockEditCommand);
	Commands_Register(&CuboidCommand);
	Commands_Register(&ReplaceCommand);
	Commands_Register(&RenameCommand);
	Commands_Register(&PosFlyCommand);
	Commands_Register(&OCuboidCommand);
	Commands_Register(&SCuboidCommand);
	Commands_Register(&LCuboidCommand);
	Commands_Register(&DCuboidCommand);
	Commands_Register(&RickRollCommand);
	Commands_Register(&N8BallCommand);
	Commands_Register(&BadAppleCommand);
	Commands_Register(&StreamCommand);
	Commands_Register(&BStreamCommand);
	Commands_Register(&LolcatCommand);
	Commands_Register(&UwUCommand);
	Commands_Register(&LeetCommand);
	Commands_Register(&GeminiCommand);
	Commands_Register(&GPTCommand);
	Commands_Register(&AntiAFKCommand);
	Commands_Register(&AutoClickCommand);
	Commands_Register(&NtpCommand);
    Commands_Register(&FullBrightCommand);
    Commands_Register(&MacroCommand);
    Commands_Register(&HaxCommand);
    Commands_Register(&NoSetBackCommand);
}

static void OnFree(void) {
	cmds_head = NULL;
}

struct IGameComponent Commands_Component = {
	OnInit, /* Init  */
	OnFree  /* Free  */
};
