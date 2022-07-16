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

// Globals 
/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a raw data read response from the 
/// Thermo CLICK real time application.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{
    // Cast the data block so we can index into the data
    IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_RT_TO_HL*) data_block;

    switch (messageData->cmd) {
        case IC_LIGHTRANGER5_CLICK_READ_SENSOR:
            // Pull the sensor data 
            Log_Debug("IC_LIGHTRANGER5_CLICK_READ_SENSOR: range: %dmm\n", messageData->range_mm);
            break;
        // Handle the other cases by doing nothing
        case IC_LIGHTRANGER5_CLICK_HEARTBEAT:
            Log_Debug("IC_LIGHTRANGER5_CLICK_HEARTBEAT\n");
            break;
        case IC_LIGHTRANGER5_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY:
            Log_Debug("IC_LIGHTRANGER5_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY: %s\n", messageData->telemetryJSON);

            // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
            if(dx_isAzureConnected()){
            dx_azurePublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), 
                        messageProperties, NELEMS(messageProperties), &contentProperties);

            }
            break;
        case IC_LIGHTRANGER5_CLICK_SET_AUTO_TELEMETRY_RATE:
            Log_Debug("IC_LIGHTRANGER5_CLICK_SET_AUTO_TELEMETRY_RATE: Set to %d seconds\n", messageData->telemtrySendRate);
            break;
        case IC_LIGHTRANGER5_CLICK_UNKNOWN:
        default:
            break;
        }
}
static void exercise_interface_handler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    Log_Debug("\n\n");

    //Code to read the sensor data in your application
    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_LIGHTRANGER5_CLICK_READ_SENSOR;
    dx_intercorePublish(&intercore_LIGHTRANGER5_click_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT));

    // Code to request telemetry data 
    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_LIGHTRANGER5_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY;
    dx_intercorePublish(&intercore_LIGHTRANGER5_click_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT));

}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
//    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));

    // Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
    dx_intercoreConnect(&intercore_LIGHTRANGER5_click_binding);

    // Code to request the real time app to automatically send telemetry data every 5 seconds
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT));

    ic_tx_block.cmd = IC_LIGHTRANGER5_CLICK_SET_AUTO_TELEMETRY_RATE;
    ic_tx_block.telemtrySendRate = 5;
    dx_intercorePublish(&intercore_LIGHTRANGER5_click_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT)); 

}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinUnsubscribe();
    dx_timerEventLoopStop();
}

int main(int argc, char *argv[])
{
    dx_registerTerminationHandler();

    if (!dx_configParseCmdLineArguments(argc, argv, &dx_config)) {
        return dx_getTerminationExitCode();
    }

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