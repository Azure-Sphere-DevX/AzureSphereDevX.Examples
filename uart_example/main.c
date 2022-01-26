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
/// Handler to check for Button Presses
/// </summary>
static DX_TIMER_HANDLER(ButtonPressCheckHandler)
{
    static GPIO_Value_Type buttonAState, buttonBState;

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    char buttonAMsg[] = "This is a test, ButtonA!";
    char buttonBMsg[] = "This is a test, ButtonB!";

    if (dx_gpioStateGet(&buttonA, &buttonAState)) {

        Log_Debug("Sending data over the uart!\n");
        dx_uartWrite(&loopBackClick1, buttonAMsg, strnlen(buttonAMsg, 32));
    }

    else if (dx_gpioStateGet(&buttonB, &buttonBState)) {
        Log_Debug("Sending data over the uart!\n");
        dx_uartWrite(&loopBackClick1, buttonBMsg, strnlen(buttonBMsg, 32));
    }
}
DX_TIMER_HANDLER_END

static void uart_rx_handler1(DX_UART_BINDING *uartBinding)
{
    // Read data from the uart here
    char rxBuffer[128 + 1];
    int bytesRead = dx_uartRead(uartBinding, rxBuffer, 128);
    if (bytesRead > 0) {
        // Null terminate the buffer before printing it to debug
        rxBuffer[bytesRead] = '\0';
        Log_Debug("RX(1) %d bytes from %s: %s\n", bytesRead,
                  uartBinding->name == NULL ? "No name" : uartBinding->name, rxBuffer);
    }
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timers.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_uartSetOpen(uart_bindings, NELEMS(uart_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
    dx_uartSetClose(uart_bindings, NELEMS(uart_bindings));
    dx_gpioSetClose(gpio_bindings, NELEMS(gpio_bindings));
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
