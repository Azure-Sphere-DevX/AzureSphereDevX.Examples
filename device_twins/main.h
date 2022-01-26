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
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:labmonitor;1"

#define NETWORK_INTERFACE "wlan0"

// Number of bytes to allocate for the JSON telemetry message for IoT Central
#define JSON_MESSAGE_BYTES 256
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};

#define LOCAL_DEVICE_TWIN_PROPERTY 64
static char copy_of_property_value[LOCAL_DEVICE_TWIN_PROPERTY];

DX_USER_CONFIG dx_config;

// Forward declarations
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_copy_string_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_desired_sample_rate_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_gpio_handler);
static DX_DECLARE_TIMER_HANDLER(report_now_handler);

static DX_MESSAGE_PROPERTY *sensorErrorProperties[] = {
    &(DX_MESSAGE_PROPERTY){.key = "appid", .value = "hvac"},
    &(DX_MESSAGE_PROPERTY){.key = "type", .value = "SensorError"},
    &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8",
                                                          .contentType = "application/json"};

/****************************************************************************************
 * Bindings
 ****************************************************************************************/
static DX_TIMER_BINDING report_now_timer = {
    .period = {5, 0}, .name = "report_now_timer", .handler = report_now_handler};

static DX_GPIO_BINDING network_connected_led = {.pin = NETWORK_CONNECTED_LED,
                                                .name = "network connected led",
                                                .direction = DX_OUTPUT,
                                                .initialState = GPIO_Value_Low,
                                                .invertPin = true};

static DX_GPIO_BINDING userLedRed = {.pin = LED_RED,
                                     .name = "userLedRed",
                                     .direction = DX_OUTPUT,
                                     .initialState = GPIO_Value_Low,
                                     .invertPin = true};

static DX_GPIO_BINDING userLedGreen = {.pin = LED_GREEN,
                                       .name = "userLedGreen",
                                       .direction = DX_OUTPUT,
                                       .initialState = GPIO_Value_Low,
                                       .invertPin = true};

static DX_GPIO_BINDING userLedBlue = {.pin = LED_BLUE,
                                      .name = "userLedBlue",
                                      .direction = DX_OUTPUT,
                                      .initialState = GPIO_Value_Low,
                                      .invertPin = true};

// All bindings referenced in the bindings sets will be initialised in the
// InitPeripheralsAndHandlers function
DX_TIMER_BINDING *timer_binding_set[] = {&report_now_timer};
DX_GPIO_BINDING *gpio_binding_set[] = {&network_connected_led, &userLedRed, &userLedGreen,
                                       &userLedBlue};

/****************************************************************************************
 * Azure IoT Device Twin Bindings
 ****************************************************************************************/
static DX_DEVICE_TWIN_BINDING dt_desired_sample_rate = {.propertyName = "DesiredSampleRate",
                                                        .twinType = DX_DEVICE_TWIN_INT,
                                                        .handler = dt_desired_sample_rate_handler};

static DX_DEVICE_TWIN_BINDING dt_sample_string = {.propertyName = "SampleString",
                                                  .twinType = DX_DEVICE_TWIN_STRING,
                                                  .handler = dt_copy_string_handler};

static DX_DEVICE_TWIN_BINDING dt_reported_temperature = {.propertyName = "ReportedTemperature",
                                                         .twinType = DX_DEVICE_TWIN_FLOAT};

static DX_DEVICE_TWIN_BINDING dt_reported_humidity = {.propertyName = "ReportedHumidity",
                                                      .twinType = DX_DEVICE_TWIN_DOUBLE};

static DX_DEVICE_TWIN_BINDING dt_reported_utc = {.propertyName = "ReportedUTC",
                                                 .twinType = DX_DEVICE_TWIN_STRING};

static DX_DEVICE_TWIN_BINDING dt_user_led_red = {.propertyName = "userLedRed",
                                                 .twinType = DX_DEVICE_TWIN_BOOL,
                                                 .handler = dt_gpio_handler,
                                                 .context = &userLedRed};

static DX_DEVICE_TWIN_BINDING dt_user_led_green = {.propertyName = "userLedGreen",
                                                   .twinType = DX_DEVICE_TWIN_BOOL,
                                                   .handler = dt_gpio_handler,
                                                   .context = &userLedGreen};

static DX_DEVICE_TWIN_BINDING dt_user_led_blue = {.propertyName = "userLedBlue",
                                                  .twinType = DX_DEVICE_TWIN_BOOL,
                                                  .handler = dt_gpio_handler,
                                                  .context = &userLedBlue};

// All device twins listed in device_twin_bindings will be subscribed to in
// the InitPeripheralsAndHandlers function
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {
    &dt_desired_sample_rate, &dt_reported_temperature, &dt_reported_humidity, &dt_reported_utc,
    &dt_sample_string,       &dt_user_led_red,         &dt_user_led_green,    &dt_user_led_blue};
