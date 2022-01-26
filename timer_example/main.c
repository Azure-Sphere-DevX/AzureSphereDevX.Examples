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
/// One shot timer handler example
/// </summary>
static DX_TIMER_HANDLER(oneShotHandler)
{
    dx_Log_Debug("Hello from the oneshot timer. Reloading the oneshot timer period\n");
    // The oneshot timer will trigger again in 2.5 seconds
    dx_timerOneShotSet(&oneShotTimer, &(struct timespec){2, 500 * ONE_MS});
}
DX_TIMER_HANDLER_END

/// <summary>
/// Periodic timer handler example
/// </summary>
static DX_TIMER_HANDLER(PeriodicHandler)
{
    dx_Log_Debug("Hello from the periodic timer called every 6 seconds\n");
}
DX_TIMER_HANDLER_END

/// <summary>
///  Initialize peripherals, device twins, direct methods, timers.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_Log_Debug_Init(debug_msg_buffer, sizeof(debug_msg_buffer));
    dx_timerSetStart(timers, NELEMS(timers));
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timers, NELEMS(timers));
    dx_timerEventLoopStop();
}

int main(void)
{
    dx_registerTerminationHandler();
    InitPeripheralsAndHandlers();

    // Call to run the event loop is a blocking call until termination is requested
    dx_eventLoopRun();

    ClosePeripheralsAndHandlers();
    return dx_getTerminationExitCode();
}