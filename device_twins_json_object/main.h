#pragma once

#include "hw/azure_sphere_learning_path.h" // Hardware definition

#include "app_exit_codes.h"
#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_device_twins.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"

#include <applibs/log.h>
#include <time.h>

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID ""

#define NETWORK_INTERFACE "wlan0"

// Number of bytes to allocate for the JSON telemetry message for IoT Central
#define JSON_MESSAGE_BYTES 256
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};
#define MAX_STRING_LEN 64

DX_USER_CONFIG dx_config;

// Forward declarations
static void dt_json_object_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);

/****************************************************************************************
 * Bindings
 ****************************************************************************************/

/****************************************************************************************
 * Azure IoT Device Twin Bindings
 ****************************************************************************************/                                                 
static DX_DEVICE_TWIN_BINDING dt_sample_json_object = {.propertyName = "SampleJsonObject",
                                                  .twinType = DX_DEVICE_TWIN_JSON_OBJECT,
                                                  .handler = dt_json_object_handler};

// All device twins listed in device_twin_bindings will be subscribed to in
// the InitPeripheralsAndHandlers function
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_sample_json_object};