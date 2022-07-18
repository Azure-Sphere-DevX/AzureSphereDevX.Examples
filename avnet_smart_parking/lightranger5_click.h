#pragma once

/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */
   
#define JSON_STRING_MAX_SIZE 100

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum
{
	IC_LIGHTRANGER5_CLICK_UNKNOWN,
	IC_LIGHTRANGER5_CLICK_HEARTBEAT,
	IC_LIGHTRANGER5_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_LIGHTRANGER5_CLICK_SET_AUTO_TELEMETRY_RATE,
	IC_LIGHTRANGER5_CLICK_READ_SENSOR
} INTER_CORE_CMD_LIGHTRANGER5_CLICK;
typedef uint8_t cmdType;

// Define the expected data structure. 
typedef struct  __attribute__((packed))
{
    uint8_t cmd;
    uint32_t telemtrySendRate;
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
} IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT;

typedef struct  __attribute__((packed))
{
    uint8_t cmd;
    uint32_t telemtrySendRate;
    char telemetryJSON[JSON_STRING_MAX_SIZE];  
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
    int range_mm;
} IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_RT_TO_HL;