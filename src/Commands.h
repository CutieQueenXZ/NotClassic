#ifndef CC_COMMANDS_H
#define CC_COMMANDS_H
#include "Core.h"
CC_BEGIN_HEADER

/* Executes actions in response to certain chat input
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Commands_Component;

cc_bool Commands_Execute(const cc_string* input);

/* This command is only available in singleplayer */
#define COMMAND_FLAG_SINGLEPLAYER_ONLY 0x01
/* args is passed as a single string instead of being split by spaces */
#define COMMAND_FLAG_UNSPLIT_ARGS 0x02

struct ChatCommand;
/* Represents a client-side command/action */
struct ChatCommand {
	const char* name;         /* Full name of this command */
	/* Function pointer for the actual action the command performs */
	void (*Execute)(const cc_string* args, int argsCount);
	cc_uint8 flags;           /* Flags for handling this command (see COMMAND_FLAG defines) */
	const char* help[5];      /* Messages to show when a player uses /help on this command */
	struct ChatCommand* next; /* Next command in linked-list of client commands */
};

extern cc_bool NoSetBack_enabled;

extern cc_bool NoPush_enabled;

extern cc_bool ChatGPT_Send(const cc_string* msg);

void ChatGPT_Tick(void);
void LCuboidCommand_Tick(void);
void DCuboidCommand_Tick(void);
void BadApple_Tick(void);
void Stream_Tick(void);
void Gemini_Tick(void);
void N8Ball_TickHook(void);
extern void Rickroll_Tick(void);
extern void SCuboidCommand_Tick(void);
void CuboidCommand_Tick(void);
void Stream_Tick(void);
void BStream_Tick(void);

/* Registers a client-side command, allowing it to be used with /client [cmd name] */
CC_API  void Commands_Register(      struct ChatCommand* cmd);
typedef void (*FP_Commands_Register)(struct ChatCommand* cmd);

/* Unregisters a previously registered client-side command. Pass the same
   struct ChatCommand* that was given to Commands_Register. Safe to call on
   a command that was never registered (no-op). */
CC_API  void Commands_Unregister(      struct ChatCommand* cmd);
typedef void (*FP_Commands_Unregister)(struct ChatCommand* cmd);

CC_END_HEADER
#endif
