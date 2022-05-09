/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 *
 * This examples shows how to marshall an event from a thread to the thread the event loop is running on.
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
/// One shot timer handler to turn off Alert LED
/// </summary>
static DX_TIMER_HANDLER(LedOffToggleHandler)
{
    dx_gpioOff(&led);
}
DX_TIMER_HANDLER_END

/// <summary>
/// Handler to check for Button Presses
/// </summary>
static DX_TIMER_HANDLER(ButtonPressCheckHandler)
{
    static GPIO_Value_Type buttonAState;

    if (dx_gpioStateGet(&buttonA, &buttonAState)) {
        dx_gpioOn(&led);
        // set oneshot timer to turn the led off after 1 second
        dx_timerOneShotSet(&ledOffOneShotTimer, &(struct timespec){1, 0});
    }
}
DX_TIMER_HANDLER_END

/// <summary>
/// Handler for dev boards with no onboard buttons - blink the LED every 500ms
/// </summary>
static DX_TIMER_HANDLER(BlinkLedHandler)
{
#ifdef OEM_SEEED_STUDIO_MINI

    static bool toggleLed = false;
    toggleLed = !toggleLed;
    dx_gpioStateSet(&led, toggleLed);

#endif
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(led_handler){
    static bool led_state = true;

    dx_gpioStateSet(&led, led_state);

    led_state = !led_state;
}
DX_TIMER_HANDLER_END

DX_ASYNC_HANDLER(async_test2_handler, handle)
{
    // int value = *((int *)handle->data);
    // Log_Debug("Data2:%d\n", value);
    dx_timerOneShotSet(&tmr_led, &(struct timespec){0,1});
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_test_handler, handle)
{
    // int value = *((int *)handle->data);
    // Log_Debug("Data1:%d\n", value);
    dx_timerOneShotSet(&tmr_led, &(struct timespec){0,1});
}
DX_ASYNC_HANDLER_END

static void *count_thread(void *arg)
{
    int count = 0;

    while (true) {
        count++;
        if (count % 2 == 0) {
            dx_asyncSend(&async_test, (void *)&count);
        } else {
            dx_asyncSend(&async_test2, (void *)&count);
        }
        nanosleep(&(struct timespec){0, 20 * ONE_MS}, NULL);
    }
    return NULL;
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timers.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_gpioSetOpen(gpio_set, NELEMS(gpio_set));
    dx_asyncSetInit(asyncSet, NELEMS(asyncSet));
    dx_timerSetStart(timerSet, NELEMS(timerSet));

    dx_startThreadDetached(count_thread, NULL, "count_thread");
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timerSet, NELEMS(timerSet));
    dx_gpioSetClose(gpio_set, NELEMS(gpio_set));
    dx_timerEventLoopStop();
}

int main(void)
{
    dx_registerTerminationHandler();
    InitPeripheralsAndHandlers();

    // Main loop
    while (!terminationRequired) {
        int result = EventLoop_Run(dx_timerGetEventLoop(), -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == -1 && errno != EINTR) {
            dx_terminate(DX_ExitCode_Main_EventLoopFail);
        } else if (asyncEventReady) {
            dx_asyncRunEvents();
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return dx_getTerminationExitCode();
}