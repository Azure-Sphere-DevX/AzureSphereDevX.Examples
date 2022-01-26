#pragma once

#include "hw/pwm_example.h" // Hardware definition

#include "app_exit_codes.h"
#include "dx_pwm.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <applibs/log.h>

// Forward declarations
static DX_DECLARE_TIMER_HANDLER(update_led_pwm_handler);

/***********************************************************************************************************
 * declare pwm bindings
 **********************************************************************************************************/

static DX_PWM_CONTROLLER pwm_led_controller = {.controllerId = PWM_LED_CONTROLLER,
                                               .name = "PWM Click Controller"};

static DX_PWM_BINDING pwm_red_led = {
    .pwmController = &pwm_led_controller, .channelId = 0, .name = "led"};
static DX_PWM_BINDING pwm_green_led = {
    .pwmController = &pwm_led_controller, .channelId = 1, .name = "green led"};
static DX_PWM_BINDING pwm_blue_led = {
    .pwmController = &pwm_led_controller, .channelId = 2, .name = "blue led"};
static DX_PWM_BINDING pwm_user_led = {
    .pwmController = &pwm_led_controller, .channelId = 3, .name = "user led on Seeed Mini"};

/****************************************************************************************
 * Timer Bindings
 ****************************************************************************************/

static DX_TIMER_BINDING tmr_update_led_pwm = {
    .period = {1, 0}, .name = "tmr_update_led_pwm", .handler = update_led_pwm_handler};

static DX_TIMER_BINDING *timer_bindings[] = {&tmr_update_led_pwm};
static DX_PWM_BINDING *pwm_bindings[] = {&pwm_red_led, &pwm_green_led, &pwm_blue_led,
                                         &pwm_user_led};
