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

static DX_TIMER_HANDLER(tmr_get_current_data_handler)
{
    Log_Debug("Read MCP511 device\n");
    transmit_MCP39F511_commands();
}
DX_TIMER_HANDLER_END

static void uart_rx_handler(DX_UART_BINDING *uartBinding)
{

	// Define indexes into the response message.
	enum response_msg {
		response_id = 0,
		message_size = 1,
		voltage_rms = 2,
		line_frequency = 4,
		input_voltage = 6,
		power_factor = 8,
		current_rms = 10,
	};

	static struct timeval  lastTime;
	static struct timeval  newTime;

	int productArrayIndex = NO_PRODUCT_FOUND;

    uint8_t msg[32];
    int bytesRead = dx_uartRead(uartBinding, msg, 32);
    Log_Debug("RX %d bytes from UART: ", bytesRead);

    for(int i = 0; i < 8; i++){
        Log_Debug("0x%X ", msg[i]);
    }

    Log_Debug("\n");
    return; 

	// Update the fLast<metric> variables
	fLastVoltage = (!isnan(fVoltage)) ? fVoltage : fLastVoltage;
	fLastCurrent = (!isnan(fCurrent)) ? fCurrent : fLastCurrent;
	fLastFrequency = (!isnan(fFrequency)) ? fFrequency : fFrequency;

	// NAN out all the values so we can validate we get fresh values	
	fVoltage = NAN;
	fCurrent = NAN;
	fFrequency = NAN;

	// Parse out the measurements from the mesage
	fVoltage = (float)((uint16_t)(msg[voltage_rms + 1] << 8) | ((uint16_t)(msg[voltage_rms]))) / (float)10.0f;
	fCurrent = (float)((uint32_t)(msg[current_rms + 3] << 24) | (uint32_t)(msg[current_rms + 2] << 16) | (uint32_t)(msg[current_rms + 1] << 8) | (uint32_t)(msg[current_rms])) / (float)10000.0f;
	fFrequency = (float)((uint16_t)(msg[line_frequency + 1] << 8) | ((uint16_t)(msg[line_frequency]))) / 1000.0f;

	static double deviceRunTime = 0.0F;

	// We only want to send up fresh telemetry data if either the bDeviceIsOn variable toggles, or
	// if our fMaxTimeBetweenD2CMessages is exceeded.
	
	// Check to make sure we read data into all the variables
	if (!isnan(fVoltage) && !isnan(fCurrent ) && !isnan(fFrequency)) {

		// Check the device on/off status
		bDeviceIsOn = (fCurrent > fMinCurrentThreshold + ON_OFF_LEAKAGE) ? true : false;

		// If the devcie on state changed, then do good stuff
		if (bDeviceIsOn != bLastDeviceIsOn) {
			
			// The device transitioned to On, capture the on time
			if (bDeviceIsOn) {

				// Capture the start time
				gettimeofday(&devStartTime, NULL);
			}

			else {  // !bDeviceisOn capture the stop time and calculate deviceRunTime

				gettimeofday(&devStopTime, NULL);

				deviceRunTime = (float)(devStopTime.tv_usec - devStartTime.tv_usec) / 1000000 +
					(float)(devStopTime.tv_sec - devStartTime.tv_sec);
				
				Log_Debug("deviceRunTime: %.3f\n", deviceRunTime);

				//  Iterate throught he productArray and find the product/size window that matches the 
				for (productArrayIndex = 0; productArrayIndex < sizeof(productArray) / sizeof(product_t); productArrayIndex++) {

//					Log_Debug("productArrayIndex: %d, Min: %.4f, Max: %.4f\n", productArrayIndex, productArray[productArrayIndex].minRunTime, productArray[productArrayIndex].maxRunTime);

					if ((deviceRunTime >= productArray[productArrayIndex].minRunTime) && (deviceRunTime <= productArray[productArrayIndex].maxRunTime)) {
						break;
					}
				}

				if (productArrayIndex >= sizeof(productArray) / sizeof(product_t)) {
					productArrayIndex = NO_PRODUCT_FOUND;
				}
			}

//			checkAndSendTelemetry(productArrayIndex);
			bLastDeviceIsOn = bDeviceIsOn;
		}

		if (bDeviceIsOn) {

			gettimeofday(&devStopTime, NULL);

			deviceRunTime = (float)(devStopTime.tv_usec - devStartTime.tv_usec) / 1000000 +
				(float)(devStopTime.tv_sec - devStartTime.tv_sec);

			if (deviceRunTime >= productArray[0].minRunTime && bDeviceIsOn) {
				bBrewing = true;
			}

		}
		else {
			bBrewing = false;
		}

		// Check the elapsed time since we sent the last telemetry data to Azure
		
		gettimeofday(&newTime, NULL);
		float deltaTime = (float)(newTime.tv_usec - lastTime.tv_usec) / 1000000 +
			(float)(newTime.tv_sec - lastTime.tv_sec);

		// If the max time has elapsed or of the on/off state changed, then send the data!
		if ((deltaTime >= fMaxTimeBetweenD2CMessages)) {

			// Used to determine deltaTime
			lastTime = newTime;
//			checkAndSendTelemetry(productArrayIndex);

		}
	}
}

static void transmit_MCP39F511_commands(void) {

	// Define the message to send.  We are requesting a register read starting at address 0x0006 and reading 0x0C bytes
    const uint8_t messageToSend[] = { 0xA5, 0x08, 0x41, 0x00, 0x06, 0x4E, 0x0C, 0x4E };
	uint8_t messagePart[1];

	// Setup ts to ~0.03 seconds
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 50000000;

    Log_Debug("TX:                   ");

	// The MCP39F511 does not correctly receive our message unless we send one byte at a time and insert a delay between bytes.

	for (int i = 0; i < 8; i++) {
		messagePart[0] = messageToSend[i];
        dx_uartWrite(&mcp511Uart, messagePart, 1);
		nanosleep(&ts, NULL);
        Log_Debug("0x%X ", messageToSend[i]);

	}

    Log_Debug("\n");
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
    dx_uartSetOpen(uart_bindings, NELEMS(uart_bindings));

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
    dx_uartSetClose(uart_bindings, NELEMS(uart_bindings));
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