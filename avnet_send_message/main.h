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
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:labmonitor;1"
#define NETWORK_INTERFACE "wlan0"

// Forward declarations
static void publish_message_handler(EventLoopTimer *eventLoopTimer);
static void dt_desired_temperature_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);

DX_USER_CONFIG dx_config;

/****************************************************************************************
 * Telemetry message buffer property sets
 ****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
#define JSON_MESSAGE_BYTES 256+128
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};

double desired_temperature = 0.0;

static DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "hvac"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

/****************************************************************************************
 * Timer Bindings
 ****************************************************************************************/
static DX_TIMER_BINDING publish_message = {.period = {5, 0}, .name = "publish_message", .handler = publish_message_handler};

// All timers referenced in timers with be opened in the InitPeripheralsAndHandlers function
DX_TIMER_BINDING *timers[] = {&publish_message};

/****************************************************************************************
 * Device Twins Bindings
 ****************************************************************************************/
static DX_DEVICE_TWIN_BINDING dt_desired_temperature = {
    .propertyName = "DesiredTemperature", .twinType = DX_DEVICE_TWIN_DOUBLE, .handler = dt_desired_temperature_handler};
static DX_DEVICE_TWIN_BINDING dt_reported_temperature = {.propertyName = "ReportedTemperature", .twinType = DX_DEVICE_TWIN_DOUBLE};

// All device twins referenced in timers with be opened in the InitPeripheralsAndHandlers function
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_reported_temperature, &dt_desired_temperature};