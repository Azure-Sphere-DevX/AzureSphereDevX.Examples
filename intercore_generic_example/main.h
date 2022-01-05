#pragma once

#include "hw/azure_sphere_learning_path.h" // Hardware definition

#include "app_exit_codes.h"
#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_intercore.h"
#include "dx_version.h"
#include <applibs/log.h>
#include "intercore_generic.h"

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID ""
#define NETWORK_INTERFACE "wlan0"
#define SAMPLE_VERSION_NUMBER "1.0"

// Forward declarations
static void dt_desired_sample_rate_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void dt_auto_telemetry_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);

static void request_telemetry_handler(EventLoopTimer *eventLoopTimer);
static void IntercoreResponseHandler(void *data_block, ssize_t message_length);

DX_USER_CONFIG dx_config;

static DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

// Declare two generic RTApp interfaces
#define RTAPP1_COMPONENT_ID "f6768b9a-e086-4f5a-8219-5ffe9684b001"
#define RTAPP2_COMPONENT_ID "f6768b9a-e086-4f5a-8219-5ffe9684b002"

IC_COMMAND_BLOCK_GENERIC_HL_TO_RT ic_tx_block = {.cmd = IC_GENERIC_UNKNOWN};
IC_COMMAND_BLOCK_GENERIC_RT_TO_HL ic_recv_block = {.cmd = IC_GENERIC_UNKNOWN};

DX_INTERCORE_BINDING intercore_app1 = {.nonblocking_io = true,
                                       .rtAppComponentId = RTAPP1_COMPONENT_ID,
                                       .interCoreCallback = IntercoreResponseHandler,
                                       .intercore_recv_block = &ic_recv_block,
                                       .intercore_recv_block_length = sizeof(ic_recv_block)};

DX_INTERCORE_BINDING intercore_app2 = {.nonblocking_io = true,
                                       .rtAppComponentId = RTAPP2_COMPONENT_ID,
                                       .interCoreCallback = IntercoreResponseHandler,
                                       .intercore_recv_block = &ic_recv_block,
                                       .intercore_recv_block_length = sizeof(ic_recv_block)};

// declare all bindings
static DX_DEVICE_TWIN_BINDING dt_desired_sample_rate = {.propertyName = "telemetryTimerAllApps", 
                                                        .twinType = DX_DEVICE_TWIN_INT, 
                                                        .handler = dt_desired_sample_rate_handler};

static DX_DEVICE_TWIN_BINDING dt_rtApp1_auto_telemetry_period = {.propertyName = "rtApp1AutoTelemetryTimer", 
                                                                 .twinType = DX_DEVICE_TWIN_INT, 
                                                                 .handler = dt_auto_telemetry_handler, 
                                                                 .context = &intercore_app1};
static DX_DEVICE_TWIN_BINDING dt_rtApp2_auto_telemetry_period = {.propertyName = "rtApp2AutoTelemetryTimer", 
                                                                 .twinType = DX_DEVICE_TWIN_INT, 
                                                                 .handler = dt_auto_telemetry_handler, 
                                                                 .context = &intercore_app2};

static DX_TIMER_BINDING tmr_publish_message = {.period = {10, 0}, .name = "tmr_publish_message", .handler = request_telemetry_handler};

// All bindings referenced in the folowing binding sets are initialised in the InitPeripheralsAndHandlers function
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_desired_sample_rate, &dt_rtApp1_auto_telemetry_period, &dt_rtApp2_auto_telemetry_period};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_publish_message};