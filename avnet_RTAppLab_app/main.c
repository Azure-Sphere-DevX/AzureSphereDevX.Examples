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
 *  How to use this sample
 * 
 *    Developers can use this sample as a starting point for their DevX based Azure Sphere 
 *    application.  It will connect to an Azure IoTHub, IOTCentral or Avnet's IoTConnect.  
 * 
 *    There are sections marked with "TODO" that the developer can review for hints on where
 *    to add code, or to enable code that may be needed for general support, such as sending
 *    telemetry.
 * 
 ************************************************************************************************/
#include "main.h"

static DX_DECLARE_TIMER_HANDLER(IntercoreSendLedUpdateHandler);
static DX_DECLARE_TIMER_HANDLER(IntercoreSendStringUpdateHandler);

/****************************************************************************************
 * Implementation
 ****************************************************************************************/
/// <summary>
//// Toggle the LED state and send the new state to the real-time app to turn the LED on or off
/// </summary>
static DX_TIMER_HANDLER(IntercoreSendLedUpdateHandler)
{
    static bool ledState = true;

    Log_Debug("New LED State: %s\n", ledState ? "true": "false");
    ledState = !ledState; 

    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT));

    // Set the command 
    ic_tx_block.cmd = IC_SAMPLE_CONTROL_LED;
    
    // Copy the new ledState into the ic_block
    ic_tx_block.ledOn = ledState;

    // Send the message to the real-time application
    dx_intercorePublish(&intercore_app_asynchronous, &ic_tx_block,
                            sizeof(IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT));

}
DX_TIMER_HANDLER_END

/// <summary>
/// Construct a string and send it to the real-time app.  We expect the real-time app to 
/// transfer the string over a UART (looped back to itself), reverse the string and send
/// the reversed string back to this high-level application.
/// </summary>
static DX_TIMER_HANDLER(IntercoreSendStringUpdateHandler)
{

    // Setup a char array to hold the string we send to the real-time app
    static char dynamicString[128+1] = {0};
    static int currentStringLenth = 1;

    char currentChar = ' '; // 0x20) < 0x5f)

    for(int i = 0; i < currentStringLenth; i++){
        dynamicString[i] = currentChar++;

        if(currentChar >= '^'){
            currentChar = ' ';
        }
    }

    dynamicString[currentStringLenth+1] = '\0';
    Log_Debug("TX: \"%s\"\n", dynamicString);

    if(++currentStringLenth > 128){
        currentStringLenth = 1;
    }

    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT));

    // Set the command 
    ic_tx_block.cmd = IC_SAMPLE_TX_STRING;
    
    // Copy the dynamic string into the ic_block
    strncpy(ic_tx_block.stringData, dynamicString, strnlen(dynamicString, 128));
    
    // Send the message to the real-time application
    dx_intercorePublish(&intercore_app_asynchronous, &ic_tx_block,
                            sizeof(IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT));

}
DX_TIMER_HANDLER_END

/// <summary>
/// Callback handler for Asynchronous Inter-Core Messaging Pattern
/// </summary>
static void IntercoreResponseHandler(void *data_block, ssize_t message_length)
{
    IC_COMMAND_BLOCK_LAB_CMDS_RT_TO_HL *ic_rx_block = (IC_COMMAND_BLOCK_LAB_CMDS_RT_TO_HL *)data_block;

    switch (ic_rx_block->cmd) {
    case IC_SAMPLE_UNKNOWN:
    case IC_SAMPLE_HEARTBEAT:
    case IC_SAMPLE_READ_SENSOR_RESPOND_WITH_TELEMETRY:
	case IC_SAMPLE_SET_AUTO_TELEMETRY_RATE:
    case IC_SAMPLE_READ_SENSOR:
    case IC_SAMPLE_CONTROL_LED:
        // Handle these cases, but don't do anything
        break;
    case IC_SAMPLE_TX_STRING:
        Log_Debug("RX: \"%s\"\n", ic_rx_block->stringData);
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
    // Open the connection to the real-time application
    if(!dx_intercoreConnect(&intercore_app_asynchronous)){
        Log_Debug("Inter-core connection failed . . .\n");
        Log_Debug(". . . Verify RTApp is running before starting this high-level application\n");
        Log_Debug(". . . Verify the RTApp component ID matches the Component ID in the inter-core binding in this application\n");
        dx_terminate(APP_ExitCode_InterCore_Connect_Error);
    }

    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinUnsubscribe();
    dx_directMethodUnsubscribe();
    dx_gpioSetClose(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerEventLoopStop();
}

int main(int argc, char *argv[])
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