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
 *   Low Power Temperature and Humidity Device
 * 
 *   This application was designed to wake up every powerDownPeriod seconds and  . . . 
 *   1. Connect to Avnet's IoT Connect Platform
 *   2. Read the temperature, humidity and pressure from the PHT Click Board in Click Socket #2
 *   3. Send telemetry with the sendor data
 *   3. Wait for . . 
 *   3.1 Confirmation that the telemetry message was received by the IoT Hub
 *   3.2 Check to see if there is a pending OTA update
 *   4. Power down for powerDownPeriod seconds
 * 
 *   Device Twins implemented
 *   sleepPeriodMinutes: Takes an integer with the number of minutes to sleep between sensor readings
 * 
 *   LED Operation
 *   Green: Application has started
 *   Blue: Application connected to IoT Connect and sent a telemetry message with sensor data
 *   Red: Application received confirmation that the telemtry data was received at the IoT Hub and device is going to sleep
 *   Green/Red blinking: Device is receiving a OTA update.  Device will sleep once the update is completed
 * 
 *   Validation
 *   I did my best to debug the OTA delay logic, it's difficult to do without having the debugger.  Application updates
 *   happen without any issues.  I have not testing the OTA delay with OS updates, but I think it should work fine.
 * 
 *   OTA Debug
 *   If you enable INCLUDE_OTA_DEBUG in main.h you'll see OTA messages from the Click Socket #1 UART Tx signal.  
 *   Connect your terminal using 115200, 8, N, 1.
 * 
 ************************************************************************************************/
#include "main.h"

// Globals
unsigned int powerDownPeriod = 60; // Seconds
SysEvent_Status updateStatus = SysEvent_Status_Invalid;

/// <summary>
/// Device twin handler for sleepPeriodMinutes device twin.
/// </summary>
static DX_DEVICE_TWIN_HANDLER(dtSleepPeriodHandler, deviceTwinBinding)
{
    int temp = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (IN_RANGE(temp, 1, 24*60)){ // 1 minute to 24 hours

        powerDownPeriod = (unsigned int)(temp*60);  // Convert the incomming minutes to seconds
        Log_Debug("New sleep period: %d Seconds\n", powerDownPeriod);

        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
    } 
}
DX_DEVICE_TWIN_HANDLER_END

/// <summary>
/// Algorithm to determine if a deferred update can proceed
/// </summary>
/// <param name="max_deferral_time_in_minutes">The maximum number of minutes you can defer</param>
/// <returns>Return 0 to start update, return greater than zero to defer</returns>
static uint32_t DeferredUpdateCalculate(uint32_t max_deferral_time_in_minutes, SysEvent_UpdateType type,
                                        SysEvent_Status status, const char *typeDescription,
                                        const char *statusDescription)
{
    // We don't really want to defer the update, so return 0 to kick off the update now
    return 0;
}

/// <summary>
/// Callback notification on deferred update state
/// </summary>
/// <param name="type"></param>
/// <param name="typeDescription"></param>
/// <param name="status"></param>
/// <param name="statusDescription"></param>
static void DeferredUpdateNotification(uint32_t max_deferral_time_in_minutes, SysEvent_UpdateType type,
                                       SysEvent_Status status, const char *typeDescription,
                                       const char *statusDescription)
{
    Log_Debug("Max minutes for deferral: %i, Type: %s, Status: %s", max_deferral_time_in_minutes, typeDescription,
              statusDescription);

#ifdef INCLUDE_OTA_DEBUG
    dx_uartWrite(&debugClick1, "DeferredUpdateNotification!\n\r", 32);
    dx_uartWrite(&debugClick1, (char*)typeDescription, strlen(typeDescription));
    dx_uartWrite(&debugClick1, (char*)statusDescription, strlen(statusDescription));
#endif 

    // Set the global updateStatus variable to reflect the update status.  We'll look at this 
    // variable to determine if we can sleep right away or wait for the OTA update to complete
    updateStatus = status;   
}

/**************************************************************************************************
 * waitToSleepHandler
 * 
 * This routine checks to see if we can go to sleep again.
 * 
 * In order to go back to sleep, the application should have . . 
 * 
 * 1. Connected to the IoTHub
 * 2. Read the temp/humidity sensor
 * 3. Sent at least one telemetry message
 * 4. Not be receiving an OTA update
 *************************************************************************************************/
static DX_TIMER_HANDLER(waitToSleepHandler)
{
    static bool redIsOn = true;

    // Was the telemetry message received by the IoT Hub, if so the outstanding message count
    // will be zero
    if(dx_azureGetOutstandingMessageCount() == 0){

        dx_gpioOff(&ledBlue);

        // Look at the update status variable to determine if we're getting an update.  If so, don't sleep but
        // toggle the red/green LEDs to show we're updating
        switch (updateStatus) {
            case SysEvent_Status_Pending:

#ifdef INCLUDE_OTA_DEBUG
                dx_uartWrite(&debugClick1, "Update Pending!\n\r", 32);
#endif 
                if(redIsOn){
                    dx_gpioOff(&ledRed);
                    dx_gpioOn(&ledGreen);
                } else 
                {
                    dx_gpioOn(&ledRed);
                    dx_gpioOff(&ledGreen);              
                }
                redIsOn = !redIsOn;
                return;
            default:
            case SysEvent_Status_Final:
            case SysEvent_Status_Complete:
            case SysEvent_Status_Deferred:
            case SysEvent_Status_Invalid:

#ifdef INCLUDE_OTA_DEBUG
                dx_uartWrite(&debugClick1, "PowerDown!\n\r", 32);
#endif
                PowerManagement_ForceSystemPowerDown(powerDownPeriod);
        }   
    }
}
DX_TIMER_HANDLER_END

static DX_TIMER_HANDLER(waitForConnectionHandler)  
{

    // Check to see if we're connected to IoTConnect, if so read the sensor and send telemetry
    if(dx_isAvnetConnected()){

        dx_deviceTwinReportValue(&dt_version_string, "LowPowerTempHum-V1.2");
    
        memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_PHT_CLICK_HL_TO_RT));

        // Send read sensor message to realtime app, this call will block until the data is 
        // received or it times out.
        ic_tx_block.cmd = IC_PHT_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY;
        if (dx_intercorePublishThenRead(&intercore_pht_click_binding, &ic_tx_block,
                                sizeof(IC_COMMAND_BLOCK_PHT_CLICK_HL_TO_RT))< 0){
                Log_Debug("Intercore message request/response failed!\n");                        
        
        } else {

            // Cast the data block so we can index into the data
            IC_COMMAND_BLOCK_PHT_CLICK_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_PHT_CLICK_RT_TO_HL*) intercore_pht_click_binding.intercore_recv_block;

            // Verify we received the expected message response
            if (messageData->cmd == IC_PHT_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY) {
            
                Log_Debug("IC_PHT_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY: %s\n", messageData->telemetryJSON);

                // Send the telemetry, the real-time app sent back valid telemetry JSON, so just send it.
#ifdef USE_AVNET_IOTCONNECT
                dx_avnetPublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), NULL, 0, NULL, NULL);
#else // Azure IoTHub or IoTCentral
                dx_azurePublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), NULL, 0, NULL);
#endif 
                // Turn on the Blue LED to indicate that we've sent a telemetry message
                dx_gpioOn(&ledBlue);
                dx_gpioOff(&ledRed);
                dx_gpioOff(&ledGreen);

                // Extend this timer to 120 seconds, we should go back to sleep before comming in here again
                dx_timerChange(&connectionCheckTimer, &(struct timespec){120, 0 * ONE_MS});

                // Start the sleepCheckTimer timer
                dx_timerChange(&sleepCheckTimer, &(struct timespec){0, 500 * ONE_MS});

                //  Start the timer that will wait for the telemetry message to be accepted by the IoTHub before sleeping
                if(!dx_timerStart(&sleepCheckTimer)){
                    Log_Debug("Error: Failed to start sleepCheckTimer\n");
                    dx_terminate(APP_ExitCode_Sleep_Check_Timer_Start_Failed);
                }
            }
        }
    }
}
DX_TIMER_HANDLER_END

/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
#ifdef USE_AVNET_IOTCONNECT
    dx_avnetSetApiVersion(AVT_API_VERSION_2_1);
//  dx_avnetSetDebugLevel(AVT_DEBUG_LEVEL_VERBOSE);
    dx_avnetConnect(&dx_config, NETWORK_INTERFACE);
#else     
    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
#endif     
    
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));
    dx_intercoreConnect(&intercore_pht_click_binding);

#ifdef INCLUDE_OTA_DEBUG
    dx_uartSetOpen(uart_bindings, NELEMS(uart_bindings));
#endif

    // Turn on the Green LED so we know the device powered back on
    dx_gpioOn(&ledGreen);

    // Register for OTA update notifications
    dx_deferredUpdateRegistration(DeferredUpdateCalculate, DeferredUpdateNotification);

#ifdef INCLUDE_OTA_DEBUG
    dx_uartWrite(&debugClick1, "Starting up . . . \n\r", strnlen("Starting up . . . \n\r", 32));
#endif 
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