#pragma once

typedef enum
{
	IC_UNKNOWN,
	IC_ECHO
} INTER_CORE_CMD;

typedef struct
{
	INTER_CORE_CMD cmd;
	int msgId;
	char message[64];
} INTER_CORE_BLOCK;
