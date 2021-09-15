﻿/* Copyright (c) Microsoft Corporation. All rights reserved.
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
 *
 *
 * The intercore communications labs require multiple instances of VS Code to be running
 *
 * It is recommended to install the VS Code Peacock extension for the intercore communications labs.
 * The Peacock extension allows you to change the color of your Visual Studio Code workspace.
 * Ideal when you have multiple VS Code instances
 *
 * Install the Peacock extension from
 * https://marketplace.visualstudio.com/items?itemName=johnpapa.vscode-peacock
 *
 * The following colours have been set:
 * The VS Code instance attached to the Real-Time core will be red. Real-time is red, as in racing
 * red. The VS Code instance attached to the High-level core is blue. High-level is blue, as in sky
 * is high and blue. You can change the default colours to match your preferences.
 *
 *
 * Intercore messaging.
 *
 * There needs to be a shared understanding of the data structure being shared between the real-time
 * and high-level apps This shared understanding is declared in the intercore_contract.h file.  This
 * file can be found in the IntercoreContract directory.
 *
 *************************************************************************************************************************************/

#include "main.h"

/// <summary>
/// Send message to realtime core app two
/// </summary>
static void IntercoreSendMessageHandler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    // send echo message to realtime core app one
    memset(&ic_control_block_app_one, 0x00, sizeof(LP_INTER_CORE_BLOCK));
    ic_control_block_app_one.cmd = LP_IC_ECHO;    
    strncpy(ic_control_block_app_one.message, REAL_TIME_COMPONENT_ID_APP_ONE, sizeof(ic_control_block_app_one.message));

    dx_intercorePublish(&intercore_app_one, &ic_control_block_app_one, sizeof(LP_INTER_CORE_BLOCK));

    // send echo message to realtime core app two
    memset(&ic_control_block_app_two, 0x00, sizeof(LP_INTER_CORE_BLOCK));
    ic_control_block_app_two.cmd = LP_IC_ECHO;
    strncpy(ic_control_block_app_two.message, REAL_TIME_COMPONENT_ID_APP_TWO, sizeof(ic_control_block_app_two.message));

    // Intercore syncronise publish request then wait for read pattern with 1000 microsecond timeout. 
    // Typical turn around time is 100 to 250 microseconds
    if (dx_intercorePublishThenRead(&intercore_app_two, &ic_control_block_app_two, sizeof(LP_INTER_CORE_BLOCK)) >= 0) {

        LP_INTER_CORE_BLOCK *ic_message_block = (LP_INTER_CORE_BLOCK *)intercore_app_two.intercore_recv_block;

        if (ic_message_block->cmd == LP_IC_ECHO) {
             Log_Debug("Echoed message number %d from realtime core id: %s\n", ic_message_block->msgId, ic_message_block->message);
        }
    } else {
        Log_Debug("Intercore message request/response failed\n");
    }

    dx_timerOneShotSet(&intercoresSendMessageTimer, &(struct timespec){0, 250 * ONE_MS});
}

/// <summary>
/// Callback handler for Inter-Core Messaging - Does Device Twin Update, and Event Message
/// </summary>
static void IntercoreResponseHandler(void *data_block, ssize_t message_length)
{
    LP_INTER_CORE_BLOCK *ic_message_block = (LP_INTER_CORE_BLOCK *)data_block;

    switch (ic_message_block->cmd) {
    case LP_IC_ECHO:
        Log_Debug("Echoed message number %d from realtime core id: %s\n", ic_message_block->msgId, ic_message_block->message);
        break;
    default:
        break;
    }
}

/// <summary>
///  Initialize PeripheralGpios, device twins, direct methods, timers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static void InitPeripheralAndHandlers(void)
{
    dx_timerSetStart(timerSet, NELEMS(timerSet));

    // Initialize Intercore Communications for core one
    dx_intercoreConnect(&intercore_app_one);

    // Initialize Intercore Communications for core two
    dx_intercoreConnect(&intercore_app_two);
    // set intercore read timeout to 1000 microseconds
    dx_intercorePublishThenReadTimeout(&intercore_app_two, 1000);

    dx_timerOneShotSet(&intercoresSendMessageTimer, &(struct timespec){1, 0});
}

/// <summary>
/// Close PeripheralGpios and handlers.
/// </summary>
static void ClosePeripheralAndHandlers(void)
{
    dx_timerSetStop(timerSet, NELEMS(timerSet));
    dx_timerEventLoopStop();
}

int main(int argc, char *argv[])
{
    dx_registerTerminationHandler();
    InitPeripheralAndHandlers();

    // Main loop
    while (!dx_isTerminationRequired()) {
        int result = EventLoop_Run(dx_timerGetEventLoop(), -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == -1 && errno != EINTR) {
            dx_terminate(DX_ExitCode_Main_EventLoopFail);
        }
    }

    ClosePeripheralAndHandlers();

    Log_Debug("Application exiting.\n");
    return dx_getTerminationExitCode();
}