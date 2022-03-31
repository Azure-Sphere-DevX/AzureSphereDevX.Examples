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

static float lastTempMeasurement = -100.0F;
static float lastHumMeasurement = -100.0F;
static double calculatedRoomTemp = 0.0F;
static bool roomTempCalculated = false;

/****************************************************************************************
 * Implementation
 ****************************************************************************************/
/// <summary>
/// Handler to check for Button Presses
/// </summary>
static DX_TIMER_HANDLER(ButtonPressCheckHandler)
{
    static GPIO_Value_Type buttonAState;
    static GPIO_Value_Type buttonBState;

    // Check to see if either button was pressed
    if (dx_gpioStateGet(&buttonA, &buttonAState) || dx_gpioStateGet(&buttonB, &buttonBState) ) {

        // Turn the RBB LED on bright to indicate that we're sampling room temperature
        dx_pwmSetDutyCycle(&pwm_red_led, 1000, 0);
        dx_pwmSetDutyCycle(&pwm_green_led, 1000, 0);
        dx_pwmSetDutyCycle(&pwm_blue_led, 1000, 0);        

        // Set the global flag to false so we calculate room temp again
        roomTempCalculated = false;
        calculatedRoomTemp = 0.0;
    }
}
DX_TIMER_HANDLER_END

static void updateHistoricalTempData(float tempData){

    static float historicalTempData[HISTORICAL_DATA_ARRAY_SIZE];
    static int arrayIndex = 0;
    double runningAverageSum = 0;

    if(!roomTempCalculated && ++arrayIndex >= HISTORICAL_DATA_ARRAY_SIZE){
        // Calculate room temperature based on the first HISTORICAL_DATA_ARRAY_SIZE
        // temperature readings
        for(int i = 0; i < HISTORICAL_DATA_ARRAY_SIZE; i++){
            runningAverageSum = runningAverageSum + tempData;
        }
        calculatedRoomTemp = runningAverageSum/(double)HISTORICAL_DATA_ARRAY_SIZE;
        roomTempCalculated = true;
        arrayIndex = 0; // Reset the index in case we recalcualte during run time
        Log_Debug("Room temperature captured as %.2fC\n", calculatedRoomTemp);
    }
    // We're still capturing data, update the next array location with the new temperature reading
    else{
        historicalTempData[arrayIndex] = tempData;
    }
}
static DX_DEVICE_TWIN_HANDLER(dt_set_sensor_polling_period_ms, deviceTwinBinding)
{
    // validate data is sensible range before applying. 
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_INT &&
        *(int *)deviceTwinBinding->propertyValue >= MIN_POLL_TIME_MS &&
        *(int *)deviceTwinBinding->propertyValue <= MAX_POLL_TIME_MS) {

        // Break the ms value int seconds and ms for the timespec struct
        int seconds = (int)(*(int *)deviceTwinBinding->propertyValue/1000);
        int ms = (long)(*(int *)deviceTwinBinding->propertyValue) - (seconds * 1000);

        dx_timerChange(&readSensorTimer, &(struct timespec){seconds, ms * ONE_MS});

        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue,
                                     DX_DEVICE_TWIN_RESPONSE_COMPLETED);

    } else {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue,
                                     DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_set_telemetemetry_period_seconds, deviceTwinBinding)
{

    // validate data is sensible range before applying. 
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_INT &&
        *(int *)deviceTwinBinding->propertyValue >= MIN_TELEMETRY_TX_PERIOD &&
        *(int *)deviceTwinBinding->propertyValue <= MAX_TELEMETRY_TX_PERIOD) {

        dx_timerChange(&sendTelemetryTimer, &(struct timespec){ *(int *)deviceTwinBinding->propertyValue, 0});

        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue,
                                     DX_DEVICE_TWIN_RESPONSE_COMPLETED);

    } else {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue,
                                     DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}
DX_DEVICE_TWIN_HANDLER_END

// Using the rangeStatus value, turn on/off the range indication LEDs
static void setPwmStatusLed(float temp)
{
    static float lastTemp = -100.0;
    uint32_t dutyCycle = 100;

	// Nothing to see here folks, move along . . .
	if(lastTemp == temp){
		return;
	}

	// Update the local static variable
    lastTemp = temp;

	// Turn off all the LED's then set the LED corresponding to the range status
    // Turn off RGBLED - 100% duty cycle is off
    dx_pwmSetDutyCycle(&pwm_red_led, 1000, 100);
    dx_pwmSetDutyCycle(&pwm_green_led, 1000, 100);
    dx_pwmSetDutyCycle(&pwm_blue_led, 1000, 100);

    // Detmine if we're under of over the calculated room temperature, then calculate the duty cycle
    // 100% == LED Full Brightness
    // 0% == LED Off
    if(roomTempCalculated){
        if(temp < calculatedRoomTemp){
            dutyCycle = (uint32_t)((float)(temp-(calculatedRoomTemp-OVER_UNDER_RANGE))/
                                  (float)(calculatedRoomTemp-(calculatedRoomTemp-OVER_UNDER_RANGE))*100);
            dx_pwmSetDutyCycle(&pwm_blue_led, 1000, dutyCycle);
        }
        else if(temp > calculatedRoomTemp){
            dutyCycle = (uint32_t)((float)(temp-(calculatedRoomTemp+OVER_UNDER_RANGE))/
                                   (float)(calculatedRoomTemp-(calculatedRoomTemp+OVER_UNDER_RANGE))*100);
            dx_pwmSetDutyCycle(&pwm_red_led, 1000, dutyCycle);
        }
    }
}

/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a raw data read response from the 
/// Thermo CLICK real time application.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{

    // Cast the data block so we can index into the data
    IC_COMMAND_BLOCK_TEMPHUM_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_TEMPHUM_RT_TO_HL*) data_block;

    switch (messageData->cmd) {
        case IC_TEMPHUM_READ_SENSOR:
            if(roomTempCalculated){
                setPwmStatusLed(messageData->temp);
            }
            else{
                updateHistoricalTempData(messageData->temp);
            }

            lastTempMeasurement = messageData->temp;
            lastHumMeasurement = messageData->hum;

            break;
        // Handle the other cases by doing nothing
        case IC_TEMPHUM_HEARTBEAT:
        case IC_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY:
        case IC_TEMPHUM_SET_TELEMETRY_SEND_RATE:
        case IC_TEMPHUM_UNKNOWN:
        default:
            break;
    }
}

/// <summary>
/// Periodic timer to read the TMF8801 sensor
/// </summary>
static DX_TIMER_HANDLER(ReadSensorHandler)
{
    //Code to read the sensor data in your application
    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_TEMPHUM_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_TEMPHUM_READ_SENSOR;
    dx_intercorePublish(&intercore_tempHum13_click_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_TEMPHUM_HL_TO_RT));
}
DX_TIMER_HANDLER_END

/// <summary>
/// Periodic timer to read the TMF8801 sensor
/// </summary>
static DX_TIMER_HANDLER(SendTelemetryHandler)
{

    snprintf(msgBuffer, sizeof(msgBuffer), "{\"temp\":%.2f, \"humidity\": %.2f}", lastTempMeasurement, lastHumMeasurement);                
    Log_Debug("%s\n", msgBuffer);

    if(dx_isAzureConnected()){
        dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);
    }
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
    // TODO, to connect this application to Azure, remove the comment
    // specifier below and update the app_manifest.json file with the details
    // for your Azure resources and Azure Sphere tenant.    
    //dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
#endif  
    
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));
    dx_intercoreConnect(&intercore_tempHum13_click_binding);
    dx_pwmSetOpen(pwm_bindings, NELEMS(pwm_bindings));
 
    // Turn on all RGBLEDs - 0% duty cycle is 100% on
    dx_pwmSetDutyCycle(&pwm_red_led, 1000, 0);
    dx_pwmSetDutyCycle(&pwm_green_led, 1000, 0);
    dx_pwmSetDutyCycle(&pwm_blue_led, 1000, 0);

    Log_Debug("Temp&Hum13 Demo Starting . . . \n");
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    
    // Turn off RGBLEDs - 100% duty cycle is off
    dx_pwmSetDutyCycle(&pwm_red_led, 1000, 100);
    dx_pwmSetDutyCycle(&pwm_green_led, 1000, 100);
    dx_pwmSetDutyCycle(&pwm_blue_led, 1000, 100);

    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
    dx_pwmSetClose(pwm_bindings, NELEMS(pwm_bindings));
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