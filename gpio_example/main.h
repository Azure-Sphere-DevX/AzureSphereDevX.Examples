#pragma once

#include "hw/azure_sphere_learning_path.h" // Hardware definition

#include "app_exit_codes.h"
#include "dx_gpio.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <applibs/log.h>

// Forward declarations
static DX_DECLARE_TIMER_HANDLER(BlinkLedHandler);
static DX_DECLARE_TIMER_HANDLER(ButtonPressCheckHandler);
static DX_DECLARE_TIMER_HANDLER(LedOffToggleHandler);

/****************************************************************************************
 * GPIO Peripherals
 ****************************************************************************************/
static DX_GPIO_BINDING buttonA = {
    .pin = BUTTON_A, .name = "buttonA", .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW};

static DX_GPIO_BINDING led = {.pin = LED2,
                              .name = "led",
                              .direction = DX_OUTPUT,
                              .initialState = GPIO_Value_Low,
                              .invertPin = true};

// All GPIOs added to gpio_set will be opened in InitPeripheralsAndHandlers
DX_GPIO_BINDING *gpio_set[] = {&buttonA, &led};

/****************************************************************************************
 * Timer Bindings
 ****************************************************************************************/
static DX_TIMER_BINDING buttonPressCheckTimer = {
    .period = {0, 1000000}, .name = "buttonPressCheckTimer", .handler = ButtonPressCheckHandler};

static DX_TIMER_BINDING ledOffOneShotTimer = {
    .period = {0, 0}, .name = "ledOffOneShotTimer", .handler = LedOffToggleHandler};

// This is for Seeed Studio Mini as there are no onboard buttons. The LED will blink every 500 ms
static DX_TIMER_BINDING blinkLedTimer = {
    .period = {0, 500 * ONE_MS}, .name = "blinkLedTimer", .handler = BlinkLedHandler};

// All timers referenced in timers with be opened in the InitPeripheralsAndHandlers function
DX_TIMER_BINDING *timerSet[] = {&buttonPressCheckTimer, &ledOffOneShotTimer, &blinkLedTimer};
