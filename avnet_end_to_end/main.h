#pragma once

#include "hw/azure_sphere_learning_path.h" // Hardware definition

#include "app_exit_codes.h"
#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_version.h"
#include <applibs/log.h>

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:labmonitor;1"
#define NETWORK_INTERFACE "wlan0"
#define SAMPLE_VERSION_NUMBER "1.0"

// Forward declarations
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_desired_sample_rate_handler);
static DX_DECLARE_DIRECT_METHOD_HANDLER(LightControlHandler);
static DX_DECLARE_TIMER_HANDLER(publish_message_handler);
static DX_DECLARE_TIMER_HANDLER(report_properties_handler);

DX_USER_CONFIG dx_config;

/****************************************************************************************
 * Telemetry message buffer property sets
 ****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
#define JSON_MESSAGE_BYTES 256
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};

static DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "hvac"}, &(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

// declare all bindings
static DX_DEVICE_TWIN_BINDING dt_desired_sample_rate = {.propertyName = "DesiredSampleRate", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_desired_sample_rate_handler};
static DX_DEVICE_TWIN_BINDING dt_deviceConnectUtc = {.propertyName = "DeviceConnectUtc", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_deviceStartUtc = {.propertyName = "DeviceStartUtc", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_reported_humidity = {.propertyName = "ReportedHumidity", .twinType = DX_DEVICE_TWIN_DOUBLE};
static DX_DEVICE_TWIN_BINDING dt_reported_temperature = {.propertyName = "ReportedTemperature", .twinType = DX_DEVICE_TWIN_FLOAT};
static DX_DEVICE_TWIN_BINDING dt_reported_utc = {.propertyName = "ReportedUTC", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_softwareVersion = {.propertyName = "SoftwareVersion", .twinType = DX_DEVICE_TWIN_STRING};

static DX_DIRECT_METHOD_BINDING dm_light_control = {.methodName = "LightControl", .handler = LightControlHandler};
static DX_GPIO_BINDING gpio_led = {.pin = LED2, .name = "gpio_led", .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true};

static DX_GPIO_BINDING gpio_network_led = {.pin = NETWORK_CONNECTED_LED, .name = "gpio_network_led", .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true};

static DX_TIMER_BINDING tmr_publish_message = {.period = {4, 0}, .name = "tmr_publish_message", .handler = publish_message_handler};
static DX_TIMER_BINDING tmr_report_properties = {.period = {5, 0}, .name = "tmr_report_properties", .handler = report_properties_handler};

// All bindings referenced in the folowing binding sets are initialised in the InitPeripheralsAndHandlers function
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_deviceStartUtc,    &dt_softwareVersion, &dt_desired_sample_rate, &dt_reported_temperature,
                                                  &dt_reported_humidity, &dt_reported_utc,    &dt_deviceConnectUtc};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {&dm_light_control};
DX_GPIO_BINDING *gpio_bindings[] = {&gpio_network_led, &gpio_led};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_publish_message, &tmr_report_properties};