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
 *	 3. Seeed Studio Azure Sphere MT3620 Development Kit aka Reference Design Board or rdb.
 *	 4. Seeed Studio Seeed Studio MT3620 Mini Dev Board.
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
/// Simple PWM example - 1kHz (1000 Hz), and alter duty cycle
/// </summary>
static DX_TIMER_HANDLER(update_led_pwm_handler)
{
#ifdef OEM_SEEED_STUDIO_MINI
    // NO RGB LED so set PWM on the user LED
    static DX_PWM_BINDING *pwmLed = &pwm_user_led;
#else
    static DX_PWM_BINDING *pwmLed = &pwm_red_led;
#endif

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
}
DX_TIMER_HANDLER_END

/// <summary>
///  Initialize peripherals, device twins, direct methods, timers.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_pwmSetOpen(pwm_bindings, NELEMS(pwm_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));

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
    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
    dx_pwmSetClose(pwm_bindings, NELEMS(pwm_bindings));
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