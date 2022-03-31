/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#define JSON_STRING_MAX_SIZE 100

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum   __attribute__((packed))
{
	IC_TEMPHUM_UNKNOWN, 
    IC_TEMPHUM_HEARTBEAT,
	IC_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_TEMPHUM_SET_TELEMETRY_SEND_RATE, 
	/////////////////////////////////////////////////////////////////////////////////
	// Don't change the enums above or the generic RTApp implementation will break //
	/////////////////////////////////////////////////////////////////////////////////
    IC_TEMPHUM_READ_SENSOR

} INTER_CORE_CMD_TEMPHUM;

// Define the expected data structure. 
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_TEMPHUM cmd;
    uint32_t telemtrySendRate;
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
} IC_COMMAND_BLOCK_TEMPHUM_HL_TO_RT;

typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_TEMPHUM cmd;
    uint32_t telemtrySendRate;
    char telemetryJSON[JSON_STRING_MAX_SIZE];
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
    float temp;
    float hum;
} IC_COMMAND_BLOCK_TEMPHUM_RT_TO_HL;