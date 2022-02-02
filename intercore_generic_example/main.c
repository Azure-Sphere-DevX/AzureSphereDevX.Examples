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
static bool rt_app1_running = false;
static bool rt_app2_running = false;

static DX_TIMER_HANDLER(request_telemetry_handler)
{
    Log_Debug("Request telemetry from the RTApp(s)\n");

    if(rt_app1_running){

        // Send IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY message to realtime core app one
        memset(&ic_tx_block, 0x00, sizeof(ic_tx_block));
        ic_tx_block.cmd = IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY;
        dx_intercorePublish(&intercore_app1, &ic_tx_block,
                            sizeof(IC_COMMAND_BLOCK_GENERIC_HL_TO_RT));        
    }

    if(rt_app2_running){

        // Send IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY message to realtime core app two
        memset(&ic_tx_block, 0x00, sizeof(ic_tx_block));
        ic_tx_block.cmd = IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY;
        dx_intercorePublish(&intercore_app2, &ic_tx_block,
                            sizeof(IC_COMMAND_BLOCK_GENERIC_HL_TO_RT));        
    }
}
DX_TIMER_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_desired_sample_rate_handler, deviceTwinBinding)
{
    int sample_rate_seconds = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (sample_rate_seconds >= 0 && sample_rate_seconds <= 120) {

        Log_Debug("New sample rate: %d seconds\n", sample_rate_seconds);

        dx_timerChange(&tmr_request_telemetry, &(struct timespec){sample_rate_seconds, 0});
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_auto_telemetry_handler, deviceTwinBinding)
{
    int telemetry_time_seconds = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (telemetry_time_seconds >= 0 && telemetry_time_seconds <= 86400) { // 0 (off) to 24hrs

        Log_Debug("New auto telemetry rate: %d seconds\n", telemetry_time_seconds);

        // Verify that we have a non-NULL context pointer.  The pointer references a intercore binding 
        if(deviceTwinBinding->context != NULL){

            DX_INTERCORE_BINDING myBinding = *(DX_INTERCORE_BINDING *)deviceTwinBinding->context;

            // Send IC_GENERIC_SAMPLE_RATE message to realtime core app one
            memset(&ic_tx_block, 0x00, sizeof(ic_tx_block));
            ic_tx_block.cmd = IC_GENERIC_SAMPLE_RATE;
            ic_tx_block.sensorSampleRate = (uint32_t)telemetry_time_seconds;
            dx_intercorePublish(&myBinding, &ic_tx_block,
                                sizeof(IC_COMMAND_BLOCK_GENERIC_HL_TO_RT));           

        }
      
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
    }
}
DX_DEVICE_TWIN_HANDLER_END

/// <summary>
/// Callback handler for Asynchronous Inter-Core Messaging Pattern
/// </summary>
static void IntercoreResponseHandler(void *data_block, ssize_t message_length)
{
    IC_COMMAND_BLOCK_GENERIC_RT_TO_HL *ic_message_block = (IC_COMMAND_BLOCK_GENERIC_RT_TO_HL *)data_block;

    switch (ic_message_block->cmd) {
        
    case IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY:
        Log_Debug("IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY recieved\n");

        Log_Debug("Tx telemetry: %s\n", ic_message_block->telemetryJSON);

        // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
        if(dx_isAzureConnected()){
            dx_azurePublish(ic_message_block->telemetryJSON, strnlen(ic_message_block->telemetryJSON, JSON_STRING_MAX_SIZE), 
                            messageProperties, NELEMS(messageProperties), &contentProperties);

        }
        break;

    // Handle all other cases by doing nothing . . .
    case IC_GENERIC_UNKNOWN:
        Log_Debug("RX IC_GENERIC_UNKNOWN response\n");
        break;
    case IC_GENERIC_SAMPLE_RATE:
        Log_Debug("RX IC_GENERIC_SAMPLE_RATE response\n");
        break;
    case IC_GENERIC_HEARTBEAT:
        Log_Debug("RX IC_GENERIC_HEARTBEAT response\n");
        break;
    default:
        break;
    }
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));

    // Initialize asynchronous inter-core messaging
    if(dx_intercoreConnect(&intercore_app1)){
        rt_app1_running = true;

        // Force the auto Telemetry feature off
        memset(&ic_tx_block, 0x00, sizeof(ic_tx_block));
        ic_tx_block.cmd = IC_GENERIC_SAMPLE_RATE;
        ic_tx_block.sensorSampleRate = 0;
        dx_intercorePublish(&intercore_app1, &ic_tx_block,
                            sizeof(IC_COMMAND_BLOCK_GENERIC_HL_TO_RT));        

        Log_Debug("RTApp1 present and running!\n");
    }
    else{
        Log_Debug("RTApp1 Not found!\n");
    }

    // Initialize asynchronous inter-core messaging
    if(dx_intercoreConnect(&intercore_app2)){
        rt_app2_running = true;

        // Force the auto Telemetry feature off
        memset(&ic_tx_block, 0x00, sizeof(ic_tx_block));
        ic_tx_block.cmd = IC_GENERIC_SAMPLE_RATE;
        ic_tx_block.sensorSampleRate = 0;
        dx_intercorePublish(&intercore_app2, &ic_tx_block,
                            sizeof(IC_COMMAND_BLOCK_GENERIC_HL_TO_RT));        

        Log_Debug("RTApp2 present and running!\n");
    }
    else{
        Log_Debug("RTApp2 Not found!\n");
    }

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