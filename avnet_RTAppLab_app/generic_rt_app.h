/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */
#pragma once

#define JSON_STRING_MAX_SIZE 128

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum __attribute__((packed))
{
	IC_SAMPLE_UNKNOWN,
	IC_SAMPLE_HEARTBEAT,
	IC_SAMPLE_READ_SENSOR_RESPOND_WITH_TELEMETRY,
	IC_SAMPLE_SET_AUTO_TELEMETRY_RATE,
	/////////////////////////////////////////////////////////////////////////////////
	// Don't change the enums above or the generic RTApp implementation will break //
	/////////////////////////////////////////////////////////////////////////////////
	IC_SAMPLE_READ_SENSOR,
	IC_SAMPLE_CONTROL_LED,
	IC_SAMPLE_TX_STRING
} INTER_CORE_CMD_SAMPLE;

// Define the expected data structure. 
typedef struct  __attribute__((packed))
{
	INTER_CORE_CMD_SAMPLE cmd;
	uint32_t telemetrySendRate;
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
	bool ledOn;
	char stringData[128];
} IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT;

typedef struct __attribute__((packed))
{
	INTER_CORE_CMD_SAMPLE cmd;
	uint32_t telemtrySendRate;
	char telemetryJSON[JSON_STRING_MAX_SIZE];
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
	uint8_t rawData8bit;
	float rawDataFloat;
} IC_COMMAND_BLOCK_SAMPLE_RT_TO_HL;

// Struct defining the memory layout for messages developed as part of the RTApp Lab
typedef struct __attribute__((packed))
{
	INTER_CORE_CMD_SAMPLE cmd;
	bool ledOn;
	char stringData[128];
} IC_COMMAND_BLOCK_LAB_CMDS_RT_TO_HL;