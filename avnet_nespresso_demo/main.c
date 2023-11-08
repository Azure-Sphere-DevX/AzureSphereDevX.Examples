/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * 
 ************************************************************************************************/
#include "main.h"

/****************************************************************************************
 * Implementation
 ****************************************************************************************/

void printProductArray(void) {
	Log_Debug("Small : Min %0.3f, Max %0.3f\n", productArray[0].minRunTime, productArray[0].maxRunTime);
	Log_Debug("Meduim: Min %0.3f, Max %0.3f\n", productArray[1].minRunTime, productArray[1].maxRunTime);
	Log_Debug("Large : Min %0.3f, Max %0.3f\n", productArray[2].minRunTime, productArray[2].maxRunTime);
}

static DX_DEVICE_TWIN_HANDLER(dt_generic_float_handler, deviceTwinBinding)
{
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_FLOAT) {

        *(float*)deviceTwinBinding->context = *(float*)deviceTwinBinding->propertyValue;

        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);

        Log_Debug("Device twin %s updated to %.2f\n", deviceTwinBinding->propertyName, *(float*)deviceTwinBinding->context);


    } else {
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
    }
}
DX_DEVICE_TWIN_HANDLER_END

// Using the networkStatus value, turn on/off the connection status LEDs
static void setConnectionStatusLed(RGB_Status newNetworkStatus)
{
	static RGB_Status lastNetworkStatus = RGB_Invalid;

	// Nothing to see here folks, move along . . .
	if(lastNetworkStatus == newNetworkStatus){
		return;
	}

	// Update the local static variable
	lastNetworkStatus = newNetworkStatus;

	// Turn off all the LED's then set the LED corresponding to the network status
	dx_gpioOff(&red_led);
	dx_gpioOff(&green_led);
	dx_gpioOff(&blue_led);

	switch (newNetworkStatus)
	{
		case RGB_No_Network: // RED LED on
			dx_gpioOn(&red_led);
			break;
		case RGB_Network_Connected: // Green LED on
			dx_gpioOn(&green_led);
			break;
		case RGB_IoT_Hub_Connected: // Blue LED on
			dx_gpioOn(&blue_led);
			break;
		case RGB_Invalid:
		default:	
			break;		
	}
}

static DX_TIMER_HANDLER(update_network_led_handler)
{
	// Assume we don't have a network connection
    RGB_Status networkStatus = RGB_No_Network;

	// Next check for Azure connectivity staus
	if(dx_isAzureConnected()){
		networkStatus = RGB_IoT_Hub_Connected;
	}
	else if (dx_isNetworkReady()){
		networkStatus = RGB_Network_Connected;
	}
	setConnectionStatusLed(networkStatus);
}
DX_TIMER_HANDLER_END


/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_azureConnect(&dx_config, NETWORK_INTERFACE, NULL);
    
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