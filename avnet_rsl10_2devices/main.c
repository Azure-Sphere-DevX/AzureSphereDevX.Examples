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

/****************************************************************************************
 * Implementation
 ****************************************************************************************/
///<summary>
///		Handler for the RSL10 authorized MAC entries
///     This handler will add/remove/modify the specified MAC from the device list
///</summary>
static DX_DEVICE_TWIN_HANDLER(rsl10AuthorizedDTFunction, deviceTwinBinding)
{
    // Get a pointer to the incomming string
    char *property_value = (char *)deviceTwinBinding->propertyValue;

    // Cast the context pointer so we can update the MAC in the structure
    RSL10Device_t *rsl10_device = (RSL10Device_t*) deviceTwinBinding->context;

    size_t propertyLen = strnlen(property_value, RSL10_ADDRESS_LEN);

    //  Verify that the context pointer is valid
    if(rsl10_device == NULL){
        Log_Debug("Invalid context pointer\n");
        return;
    }

    // Check to see if the incomming string is empty ""
    if((propertyLen == 0) || (property_value == NULL)){                
                
        // This authorizedMac entry was just removed, update the authorized address with "", and mark it inactive
        strcpy(rsl10_device->authorizedBdAddress, "");
        
    }

    // The propery value is not NULL, validate the data
    else if ((deviceTwinBinding->twinType == DX_DEVICE_TWIN_STRING) &&
            (propertyLen == RSL10_ADDRESS_LEN-1) && 
            (dx_isStringPrintable(property_value)))
    {

        // Update the structure with the new MAC address
        strncpy(rsl10_device->authorizedBdAddress, property_value, RSL10_ADDRESS_LEN);

    }
    else{

        Log_Debug("Local copy failed. String too long or invalid data\n");
        return;
    }

    // Control gets here if the incomming data is valid.

    // Mark this device as inactive, this will be updated when we receive the next message from the device
    rsl10_device->isActive = false;
    Log_Debug("Received device update. New %s is %s\n", deviceTwinBinding->propertyName, rsl10_device->authorizedBdAddress);
    dx_deviceTwinReportValue(deviceTwinBinding, (char *)deviceTwinBinding->propertyValue);

}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(enableOnboardingDTFunction, deviceTwinBinding)
{
    // Update the global variable
    enableRSL10Onboarding = *(bool *)deviceTwinBinding->propertyValue;
    dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);

    Log_Debug("Received device update. New %s is %s\n", deviceTwinBinding->propertyName, enableRSL10Onboarding ? "true": "false");
}
DX_DEVICE_TWIN_HANDLER_END


static DX_DEVICE_TWIN_HANDLER(telemetryTimerDTFunction, deviceTwinBinding)
{

    int newPollTime = *(int *)deviceTwinBinding->propertyValue;
    
    // Validate that the data is between 1 second and 1 day
    if(IN_RANGE(newPollTime, 1, 60*60*24)){

        dx_timerChange(&tmr_send_telemetry, &(struct timespec){newPollTime, 0});
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
        Log_Debug("Received device update. New %s is %d\n", deviceTwinBinding->propertyName, newPollTime);

    } else {
        Log_Debug("New timer value is out of range: %d\n", newPollTime);
    }
}
DX_DEVICE_TWIN_HANDLER_END

// Send telemetry
static DX_TIMER_HANDLER(send_telemetry_handler)
{
    if (!dx_isAzureConnected()) {
        Log_Debug("No IoTHub connection, not sending telemetry\n");
        return;
    }

    // Call the routine that will send the telemetry
    rsl10SendTelemetry();
    Log_Debug("Send telemetry\n");
}
DX_TIMER_HANDLER_END


/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
#ifdef USE_AVNET_IOTCONNECT
    dx_avnetConnect(&dx_config, NETWORK_INTERFACE);
#else     
    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
#endif     
    
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));

    // TODO: Update this call with a function pointer to a handler that will receive connection status updates
    // see the azure_end_to_end example for an example
    // dx_azureRegisterConnectionChangedNotification(NetworkConnectionState);
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