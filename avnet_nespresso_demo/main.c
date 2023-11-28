/* Copyright (c) Avnet. All rights reserved.
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

/****************************************************************************************
 * Generic float device twin handler.  Note that the actual global variable to update
 * is referenced by the deviceTwinBinding->context pointer.  This allows a single
 * handler for all float device twins.
 ****************************************************************************************/
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

/****************************************************************************************
 * Using the passed in networkStatus value, turn on/off the connection status LEDs
 * to reflect the current connection status.
 * Red == No WiFi connection
 * Green == WiFi connected not connected to Azure
 * Blue == Connected to Azure
*****************************************************************************************/
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

/****************************************************************************************
 * Periodic timer handler to check network status and drive connction LED
*****************************************************************************************/
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

/****************************************************************************************
* Periodic timer handler to request power data from the RT app
*****************************************************************************************/
static DX_TIMER_HANDLER(tmr_get_current_data_handler)
{
//    Log_Debug("Read MCP511 device\n");

	memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT));

	// Send read sensor message to realtime app
	ic_tx_block_sample.cmd = IC_PWR_METER_READ_SENSOR;
	dx_intercorePublish(&intercore_pwr_meter_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT));
}
DX_TIMER_HANDLER_END

/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a  response from the 
/// PWR meter real time application.  Note we only handle the IC_PWR_METER_READ_SENSOR
/// response.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{
	// Cast the data block so we can index into the data
	IC_COMMAND_BLOCK_PWR_METER_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_PWR_METER_RT_TO_HL*) data_block;

	int productArrayIndex = NO_PRODUCT_FOUND;

	if (IC_PWR_METER_READ_SENSOR == messageData->cmd) {

		// Update the global variables
		fVoltage = messageData->voltage;
		fCurrent = messageData->current;

		// We only want to send up fresh telemetry data if either the bDeviceIsOn variable toggles, or
		// if our fMaxTimeBetweenD2CMessages is exceeded.
		
		// Check the device on/off status
		bDeviceIsOn = (fCurrent > fMinCurrentThreshold + ON_OFF_LEAKAGE) ? true : false;

	// If the device on state changed, to On then capture the start time
		if (bDeviceIsOn != bLastDeviceIsOn) {
				
			// The device transitioned to On, capture the on time
			if (bDeviceIsOn) {

//				Log_Debug("Device is on!\n");

				// Capture the start time
				gettimeofday(&devStartTime, NULL);
			}
		}

		// Calculate current device run time
		gettimeofday(&devRunTime, NULL);
		deviceRunTime = (float)(devRunTime.tv_usec - devStartTime.tv_usec) / 1000000 +
			(float)(devRunTime.tv_sec - devStartTime.tv_sec);

		// Set the bBrewing flag
		if (bDeviceIsOn) {

//			Log_Debug("deviceRunTime: %.2f\n", deviceRunTime);

			// Manage the brewing flag.  If we just started brewing, send telemetry
			if (deviceRunTime >= productArray[0].minRunTime && bDeviceIsOn) {
				bBrewing = true;
//				Log_Debug("Brewing\n");

				// Check if we just started brewing, if so send telemetry
				if(bBrewing != bLastBrewing){

					bLastBrewing = bBrewing;
					checkAndSendTelemetry(productArrayIndex);
				}
			}
			else {
				bBrewing = false;
				bLastBrewing = false;
//				Log_Debug("NOT Brewing\n");
			}
		}

		// If the device on state changed, then do good stuff
		if (bDeviceIsOn != bLastDeviceIsOn) {
				
			// The device transitioned to On, capture the on time
			if (bDeviceIsOn) {

//				Log_Debug("Device is on!\n");

				// Capture the start time
				gettimeofday(&devStartTime, NULL);
			}

			else {  // !bDeviceisOn capture the stop time and calculate deviceRunTime

//				Log_Debug("Device is off!\n");

				bBrewing = false;

				gettimeofday(&devStopTime, NULL);

				deviceRunTime = (float)(devStopTime.tv_usec - devStartTime.tv_usec) / 1000000 +
						(float)(devStopTime.tv_sec - devStartTime.tv_sec);
					
				Log_Debug("deviceRunTime: %.3f\n", deviceRunTime);

				//  Iterate through he productArray and find the product/size window that matches 
				for (productArrayIndex = 0; productArrayIndex < sizeof(productArray) / sizeof(product_t); productArrayIndex++) {

					Log_Debug("productArrayIndex: %d, Min: %.4f, Max: %.4f\n", productArrayIndex, productArray[productArrayIndex].minRunTime, productArray[productArrayIndex].maxRunTime);

					if ((deviceRunTime >= productArray[productArrayIndex].minRunTime) && (deviceRunTime <= productArray[productArrayIndex].maxRunTime)) {
						break;
					}
				}

				// Check the case where we did not find a match
				if (productArrayIndex >= sizeof(productArray) / sizeof(product_t)) {
					productArrayIndex = NO_PRODUCT_FOUND;
				}
			}

//			Log_Debug("Found product index %d\n", productArrayIndex);
//			Log_Debug("Send Telemetry\n");

			checkAndSendTelemetry(productArrayIndex);
			bLastDeviceIsOn = bDeviceIsOn;
		
		}

		// Check the elapsed time since we sent the last telemetry data to Azure

		gettimeofday(&newTelemetryTime, NULL);
		float deltaTime = (float)(newTelemetryTime.tv_usec - lastTelemetryTime.tv_usec) / 1000000 +
			(float)(newTelemetryTime.tv_sec - lastTelemetryTime.tv_sec);

		// If the max time has elapsed or of the on/off state changed, then send the data!
		if ((deltaTime >= fMaxTimeBetweenD2CMessages)) {

//			Log_Debug("Send telemetry timeout case\n"); 
			checkAndSendTelemetry(productArrayIndex);
		}
	}
}

///<summary>
///		Determine which telemetry message to send based on the productIndex and send the telemetry message
///</summary>
static void checkAndSendTelemetry(int productIndex)
{
	if (productIndex == NO_PRODUCT_FOUND) {

		// construct the telemetry message without the product information 
		snprintf(msgBuffer, JSON_MESSAGE_BYTES, cstrDeviceTelemetryJson, fVoltage, fCurrent, 50.0, bDeviceIsOn  ? "true" : "false", bBrewing ? "true" : "false");
	}
	else {

		// construct the telemetry message without the product information 
		snprintf(msgBuffer, JSON_MESSAGE_BYTES, cstrDeviceTelemetryProductJson, fVoltage, fCurrent, 50.0, bDeviceIsOn ? "true" : "false", productArray[productIndex].productEnum, productArray[productIndex].productSizeEnum);
	}

	Log_Debug("\nSending telemetry %s\n", msgBuffer);
    dx_azurePublish(msgBuffer, strlen(msgBuffer), NULL, 0, NULL);

	// Update the time when we last sent telemetry
	gettimeofday(&lastTelemetryTime, NULL);

}

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
	dx_intercoreConnect(&intercore_pwr_meter_binding);
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