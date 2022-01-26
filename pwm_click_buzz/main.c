/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 *
 * This example is built on the Azure Sphere DevX library.
 *   1. DevX is an Open Source community-maintained implementation of the Azure Sphere SDK samples.
 *   2. DevX is a modular library that simplifies common development scenarios.
 *        - You can focus on your solution, not the plumbing.
 *   3. DevX documentation is maintained at https://github.com/gloveboxes/AzureSphereDevX/wiki
 *	 4. The DevX library is not a substitute for understanding the Azure Sphere SDK Samples.
 *          - https://github.com/Azure/azure-sphere-samples
 *
 * DEVELOPER BOARD SELECTION
 *
 * The following developer boards are supported.
 *
 *	 1. AVNET Azure Sphere Starter Kit.
 *   2. AVNET Azure Sphere Starter Kit Revision 2.
 *
 * ENABLE YOUR DEVELOPER BOARD
 *
 * Each Azure Sphere developer board manufacturer maps pins differently. You need to select the
 *    configuration that matches your board.
 *
 * Follow these steps:
 *
 *	 1. Open CMakeLists.txt.
 *	 2. Uncomment the set command that matches your developer board.
 *	 3. Click File, then Save to auto-generate the CMake Cache.
 *
 ************************************************************************************************/

#include "main.h"

/// <summary>
/// Oneshot handler to turn off BUZZ Click buzzer
/// </summary>
/// <param name="eventLoopTimer"></param>
static DX_TIMER_HANDLER(alert_buzzer_off_handler)
{
    dx_pwmStop(&pwm_buzz_click);
}
DX_TIMER_HANDLER_END

/// <summary>
/// Simple PWM example - 1kHz (1000 Hz), and alter duty cycle
/// </summary>
static DX_TIMER_HANDLER(update_led_pwm_handler)
{
    static DX_PWM_BINDING *pwmLed = &pwm_red_led;

    static int duty_cycle = 0;

    switch (duty_cycle % 4) {
    case 0:
        dx_pwmSetDutyCycle(pwmLed, 1000, 100);
        break;
    case 1:
        dx_pwmSetDutyCycle(pwmLed, 1000, 80);
        break;
    case 2:
        dx_pwmSetDutyCycle(pwmLed, 1000, 40);
        break;
    case 3:
        dx_pwmSetDutyCycle(pwmLed, 1000, 0);
        break;
    default:
        break;
    }

    duty_cycle++;

    dx_pwmSetDutyCycle(&pwm_buzz_click, 5000, 1);
    dx_timerOneShotSet(&tmr_alert_buzzer_off_oneshot, &(struct timespec){0, 10 * ONE_MS});
}
DX_TIMER_HANDLER_END

/// <summary>
///  Initialize peripherals, device twins, direct methods, timers.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_pwmSetOpen(pwm_bindings, NELEMS(pwm_bindings));
    dx_timerSetStart(timerSet, NELEMS(timerSet));

    // Turn off RGBLED - 100% duty cycle is off
    dx_pwmSetDutyCycle(&pwm_red_led, 1000, 100);
    dx_pwmSetDutyCycle(&pwm_green_led, 1000, 100);
    dx_pwmSetDutyCycle(&pwm_blue_led, 1000, 100);
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timerSet, NELEMS(timerSet));
    dx_timerEventLoopStop();
}

int main(void)
{
    dx_registerTerminationHandler();
    InitPeripheralsAndHandlers();

    // Main loop
    while (!dx_isTerminationRequired()) {
        int result = EventLoop_Run(dx_timerGetEventLoop(), -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == -1 && errno != EINTR) {
            dx_terminate(DX_ExitCode_Main_EventLoopFail);
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return dx_getTerminationExitCode();
}