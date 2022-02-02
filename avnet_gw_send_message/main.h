#pragma once

#include "hw/azure_sphere_learning_path.h" // Hardware definition

#include "app_exit_codes.h"
#include "dx_avnet_iot_connect.h"
#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_device_twins.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <applibs/log.h>

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID ""
#define NETWORK_INTERFACE "wlan0"

// Forward declarations
static DX_DECLARE_TIMER_HANDLER(add_gw_children_handler);
static DX_DECLARE_TIMER_HANDLER(delete_gw_children_handler);
static DX_DECLARE_TIMER_HANDLER(publish_message_handler);
void sendChildDeviceTelemetry(const char* id, const char* key, float value);

DX_USER_CONFIG dx_config;

/****************************************************************************************
 * Telemetry message buffer property sets
 ****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
#define JSON_MESSAGE_BYTES 512
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};

/****************************************************************************************
 * Timer Bindings
 ****************************************************************************************/
static DX_TIMER_BINDING tmr_publish_message = {.period = {10, 0}, .name = "publish_message", .handler = publish_message_handler};
static DX_TIMER_BINDING tmr_add_gw_children = {.period = {5, 0}, .name = "add gw children", .handler = add_gw_children_handler};
static DX_TIMER_BINDING tmr_delete_gw_children = {.period = {2*60, 0}, .name = "delete gw children", .handler = delete_gw_children_handler};

// All timers referenced in timers with be opened in the InitPeripheralsAndHandlers function
DX_TIMER_BINDING *timers[] = {&tmr_publish_message, &tmr_add_gw_children, &tmr_delete_gw_children};

/****************************************************************************************
 * Device Twins Bindings
 ****************************************************************************************/

// All device twins referenced in timers with be opened in the InitPeripheralsAndHandlers function
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {};