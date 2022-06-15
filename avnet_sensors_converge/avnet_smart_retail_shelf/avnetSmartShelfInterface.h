#pragma once

/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */
   
#define JSON_STRING_MAX_SIZE 100

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum
{
	IC_SMART_SHELF_UNKNOWN,
	IC_SMART_SHELF_HEARTBEAT,
	IC_SMART_SHELF_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_SMART_SHELF_SET_AUTO_TELEMETRY_RATE,
	IC_SMART_SHELF_READ_SENSOR,
	IC_SMART_SHELF_SIMULATE_DATA
} INTER_CORE_CMD_SMART_SHELF;
typedef uint8_t cmdType;

// Define the expected data structure. 
typedef struct // __attribute__((packed))
{
    uint8_t cmd;
    uint32_t telemtrySendRate;
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
	bool simulateShelfData;
} IC_COMMAND_BLOCK_SMART_SHELF_HL_TO_RT;

typedef struct // __attribute__((packed))
{
    uint8_t cmd;
    uint32_t telemtrySendRate;
    char telemetryJSON[JSON_STRING_MAX_SIZE];  
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
    float temp;
	float hum;
	float pressure;
	int rangePeople_mm;
	int rangeShelf1_mm;
	int rangeShelf2_mm;
	bool simulateShelfData;
} IC_COMMAND_BLOCK_SMART_SHELF_RT_TO_HL;