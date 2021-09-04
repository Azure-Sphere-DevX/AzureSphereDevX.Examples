#pragma once

#include "hw/azure_sphere_learning_path.h" // Hardware definition

#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_direct_methods.h"
#include "app_exit_codes.h"
#include "dx_gpio.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <applibs/log.h>
#include <applibs/powermanagement.h>

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:labmonitor;1"
#define NETWORK_INTERFACE "wlan0"

// Forward declarations
static DX_DIRECT_METHOD_RESPONSE_CODE LightControlHandler(
    JSON_Value *json, DX_DIRECT_METHOD_BINDING *directMethodBinding, char **responseMsg);
static DX_DIRECT_METHOD_RESPONSE_CODE RestartDeviceHandler(
    JSON_Value *json, DX_DIRECT_METHOD_BINDING *directMethodBinding, char **responseMsg);
static void DelayRestartDeviceTimerHandler(EventLoopTimer *eventLoopTimer);
static void LedOffToggleHandler(EventLoopTimer *eventLoopTimer);

// Variables
DX_USER_CONFIG dx_config;

/****************************************************************************************
 * GPIO Peripherals
 ****************************************************************************************/
static DX_GPIO_BINDING led = {.pin = LED2,
                              .name = "led",
                              .direction = DX_OUTPUT,
                              .initialState = GPIO_Value_Low,
                              .invertPin = true};

// All GPIOs added to gpio_set will be opened in InitPeripheralsAndHandlers
DX_GPIO_BINDING *gpio_set[] = {&led};

/****************************************************************************************
 * Timer Bindings
 ****************************************************************************************/
static DX_TIMER_BINDING led_off_oneshot_timer = {
    .period = {0, 0}, .name = "led_off_oneshot_timer", .handler = LedOffToggleHandler};

static DX_TIMER_BINDING restart_device_oneshot_timer = {.period = {0, 0},
                                                        .name = "restart_device_oneshot_timer",
                                                        .handler = DelayRestartDeviceTimerHandler};

// All timers referenced in timers with be opened in the InitPeripheralsAndHandlers function
DX_TIMER_BINDING *timers[] = {&restart_device_oneshot_timer, &led_off_oneshot_timer};

/****************************************************************************************
 * Azure IoT Direct Method Bindings
 ****************************************************************************************/
static DX_DIRECT_METHOD_BINDING dm_light_control = {.methodName = "LightControl",
                                                    .handler = LightControlHandler};

static DX_DIRECT_METHOD_BINDING dm_restart_device = {.methodName = "RestartDevice",
                                                     .handler = RestartDeviceHandler};

// All direct methods referenced in direct_method_bindings will be subscribed to in
// the InitPeripheralsAndHandlers function
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {&dm_restart_device, &dm_light_control};