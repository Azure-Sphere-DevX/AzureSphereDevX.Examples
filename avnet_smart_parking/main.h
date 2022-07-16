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
#include "lightranger5_click.h"

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID ""
#define NETWORK_INTERFACE "wlan0"
#define SAMPLE_VERSION_NUMBER "1.0"

// Forward declarations
static void exercise_interface_handler(EventLoopTimer *eventLoopTimer);
static void receive_msg_handler(void *data_block, ssize_t message_length);

DX_USER_CONFIG dx_config;

static DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT ic_tx_block;
IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_RT_TO_HL ic_rx_block;

DX_INTERCORE_BINDING intercore_LIGHTRANGER5_click_binding = {
    .sockFd = -1,
    .nonblocking_io = true,
    .rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
    .interCoreCallback = receive_msg_handler,
    .intercore_recv_block = &ic_rx_block,
    .intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_RT_TO_HL)};

// declare all bindings

static DX_TIMER_BINDING tmr_exercise_interface = {.period = {1, 0}, .name = "tmr_exercise_interface", .handler = exercise_interface_handler};

// All bindings referenced in the folowing binding sets are initialised in the InitPeripheralsAndHandlers function
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_exercise_interface};