/* Copyright (c) Avnet Incorporated. All rights reserved.
 *   Licensed under the MIT License. 
 *
 *   DEVELOPER BOARD SELECTION
 *
 *   Each Azure Sphere developer board manufacturer maps pins differently. You need to select the
 *      configuration that matches your board.
*
 *   The following developer boards are supported.
 *
 *	   1. AVNET Azure Sphere Starter Kit.
 *     2. AVNET Azure Sphere Starter Kit Revision 2.
 * *
 *   Follow these steps:
 *
 *	   1. Open CMakeLists.txt.
 *	   2. Uncomment the set command that matches your developer board.
 *	   3. Click File, then Save to save the CMakeLists.txt file which will auto generate the
 *          CMake Cache.
 *
 *
 * ************************************************************************************************/

#include "main.h"

static DX_TIMER_HANDLER(monitor_wifi_network_handler)
{
    ReadWifiConfig(true);
}
DX_TIMER_HANDLER_END

static DX_TIMER_HANDLER(read_sensors_handler)
{
    static bool firstPass = true;

    // Read the sensors
    acceleration_g = lp_get_acceleration();
    angular_rate_dps = lp_get_angular_rate();
    lsm6dso_temperature = lp_get_temperature();
#ifdef M4_INTERCORE_COMMS
    // Send read sensor message to realtime core app one
    ic_control_block_alsPt19_light_sensor.cmd = IC_READ_SENSOR;
    dx_intercorePublish(&intercore_alsPt19_light_sensor, &ic_control_block_alsPt19_light_sensor,
                            sizeof(IC_COMMAND_BLOCK_ALS_PT19));
#endif

    if(firstPass){
#ifdef OLED_SD1306
        // Transition the OLED to the Avnet Logo screen
        oled_state = LOGO;
#endif 
        // Set the flag to false and return without displaying the sensor data or sending sensor telemetry
        firstPass = false;
        return;
    }


    if(sensor_debug_enabled){
        Log_Debug("\nLSM6DSO: Acceleration      [g]   : %.4lf, %.4lf, %.4lf\n", acceleration_g.x,
                acceleration_g.y, acceleration_g.z);
        Log_Debug("LSM6DSO: Angular rate      [dps] : %4.2f, %4.2f, %4.2f\n", angular_rate_dps.x,
                angular_rate_dps.y, angular_rate_dps.z);
        Log_Debug("LSM6DSO: Temperature1      [degC]: %.2f\n", lsm6dso_temperature);
        Log_Debug("ALSPT19: Ambient Light     [Lux] : %.2f\n", light_sensor);
    }
  	if (lps22hhDetected) {

        pressure_hPa = lp_get_pressure();

        // The ALTITUDE value calculated is actually "Pressure Altitude". This lacks correction for
        // temperature (and humidity)
        // "pressure altitude" calculator located at:
        // https://www.weather.gov/epz/wxcalc_pressurealtitude "pressure altitude" formula is defined
        // at: https://www.weather.gov/media/epz/wxcalc/pressureAltitude.pdf altitude in feet =
        // 145366.45 * (1 - (hPa / 1013.25) ^ 0.190284) feet altitude in 
        // meters = 145366.45 * 0.3048 * (1 - (hPa / 1013.25) ^ 0.190284) meters
        altitude = (float)0.3048 * (float)145366.45 * (1 - powf(((float)(pressure_hPa / 1013.25)),(float)(0.190284)));

        lps22hh_temperature = lp_get_temperature_lps22h();
        
        if(sensor_debug_enabled){
            Log_Debug("LPS22HH: Pressure          [hPa] : %.2f\n", pressure_hPa);
            Log_Debug("LPS22HH: Pressure Altitude [m]   : %.2f\n", altitude);
            Log_Debug("LPS22HH: Temperature2      [degC]: %.2f\n", lps22hh_temperature);
        }
    }
    // LPS22HH was not detected
    else {

       if(sensor_debug_enabled){
            Log_Debug("LPS22HH: Pressure          [hPa] : Not read!\n");
            Log_Debug("LPS22HH: Pressure Altitude [m]   : Not calculated!\n");
            Log_Debug("LPS22HH: Temperature       [degC]: Not read!\n");
       }
    }

    // Send the latest readings up as telemetry
    publish_message_handler();
}
DX_TIMER_HANDLER_END

static void publish_message_handler(void)
{

#ifdef IOT_HUB_APPLICATION
#ifdef USE_IOT_CONNECT
    // If we have not completed the IoTConnect connect sequence, then don't send telemetry
    if(dx_isAvnetConnected()){
#else  // !IoT Connect 
    if (dx_isAzureConnected()) {
#endif // USE_IOT_CONNECT

#ifdef USE_DEVX_SERIALIZATION
        // Serialize telemetry as JSON
#ifdef USE_IOT_CONNECT
        bool serialization_result = dx_avnetJsonSerialize(avtMsgBuffer, sizeof(avtMsgBuffer), NULL, 11, 
            DX_JSON_DOUBLE, "gX", acceleration_g.x,
            DX_JSON_DOUBLE, "gY", acceleration_g.y,
            DX_JSON_DOUBLE, "gZ", acceleration_g.z,
            DX_JSON_DOUBLE, "aX", angular_rate_dps.x,
            DX_JSON_DOUBLE, "aY", angular_rate_dps.y,
            DX_JSON_DOUBLE, "aZ", angular_rate_dps.z,
            DX_JSON_DOUBLE, "pressure", pressure_hPa,
            DX_JSON_DOUBLE, "light_intensity", light_sensor,
            DX_JSON_DOUBLE, "altitude", altitude,
            DX_JSON_DOUBLE, "temp", lsm6dso_temperature,
            DX_JSON_INT, "rssi", network_data.rssi);

        if (serialization_result) {

            Log_Debug("%s\n", avtMsgBuffer);
            dx_azurePublish(avtMsgBuffer, strlen(avtMsgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);

#else // ! IoT Connect

        bool serialization_result = dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 11, 
            DX_JSON_DOUBLE, "gX", acceleration_g.x,
            DX_JSON_DOUBLE, "gY", acceleration_g.y,
            DX_JSON_DOUBLE, "gZ", acceleration_g.z,
            DX_JSON_DOUBLE, "aX", angular_rate_dps.x,
            DX_JSON_DOUBLE, "aY", angular_rate_dps.y,
            DX_JSON_DOUBLE, "aZ", angular_rate_dps.z,
            DX_JSON_DOUBLE, "pressure", pressure_hPa,
            DX_JSON_DOUBLE, "light_intensity", light_sensor,
            DX_JSON_DOUBLE, "altitude", altitude,
            DX_JSON_DOUBLE, "temp", lsm6dso_temperature,
            DX_JSON_INT, "rssi", network_data.rssi);

        if (serialization_result) {

            Log_Debug("%s\n", msgBuffer);
            dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);

#endif  // USE_IOT_CONNECT

        } else {
            Log_Debug("JSON Serialization failed: Buffer too small\n");
        }
#else // !USE_DEVX_SERIALIZATION

        snprintf(msgBuffer, sizeof(msgBuffer),
            "{\"gX\":%.2lf, \"gY\":%.2lf, \"gZ\":%.2lf, \"aX\": %.2f, \"aY\": "
            "%.2f, \"aZ\": %.2f, \"pressure\": %.2f, \"light_intensity\": %.2f, "
            "\"altitude\": %.2f, \"temp\": %.2f,  \"rssi\": %d}",
            acceleration_g.x, acceleration_g.y, acceleration_g.z, angular_rate_dps.x,
            angular_rate_dps.y, angular_rate_dps.z, pressure_hPa, light_sensor, altitude,
            lsm6dso_temperature, network_data.rssi);                

#ifdef USE_IOT_CONNECT

            // Add the IoTConnect metadata to the seralized telemetry
            dx_avnetJsonSerializePayload(msgBuffer, avtMsgBuffer, sizeof(avtMsgBuffer), NULL);
            Log_Debug("%s\n", avtMsgBuffer);
            dx_azurePublish(avtMsgBuffer, strlen(avtMsgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);

#else // !USE_IOT_CONNECt
            Log_Debug("%s\n", msgBuffer);
            dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);
#endif // USE_IOT_CONNECT
#endif // // !USE_DEVX_SERIALIZATION                    
        }
#endif // IOT_HUB_APPLICATION    
}

static DX_DEVICE_TWIN_HANDLER(dt_desired_sample_rate_handler, deviceTwinBinding)
{
    int sample_rate_seconds = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (IN_RANGE(sample_rate_seconds, 1, 12*60*60)){ // 1 second to 10 hours

        dx_timerChange(&tmr_read_sensors, &(struct timespec){sample_rate_seconds, 0});

#ifdef USE_PNP
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
#else
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
#endif // USE_PNP

    } else {
#ifdef USE_PNP
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
#endif // USE_PNP

    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_gpio_handler, deviceTwinBinding)
{
    bool gpio_level = *(bool *)deviceTwinBinding->propertyValue;

    if(deviceTwinBinding->context != NULL){
        DX_GPIO_BINDING *gpio = (DX_GPIO_BINDING*)deviceTwinBinding->context;
        
        if(gpio_level){
            dx_gpioOn(gpio);
        }
        else{
            dx_gpioOff(gpio);
        }
#ifdef USE_PNP
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
#else
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
#endif // USE_PNP
    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_oled_message_handler, deviceTwinBinding)
{
    bool message_processed = true;
    
    // Verify we have a pointer to the global variable for this oled message
    if(deviceTwinBinding->context != NULL){

        char* ptr_oled_variable = (char*)deviceTwinBinding->context;
        char* new_message = (char*)deviceTwinBinding->propertyValue;

        // Is the message size less than the destination buffer size and printable characters
        if (strlen(new_message) < CLOUD_MSG_SIZE && dx_isStringPrintable(new_message)) {
            strncpy(ptr_oled_variable, new_message, CLOUD_MSG_SIZE);
#ifdef USE_PNP
            dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
#else
            dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
#endif // USE_PNP
        } else {
            message_processed = false;
        }

    } else {
        message_processed = false;
    }

    if (!message_processed){

        Log_Debug("Local copy failed. String too long or invalid data\n");
#ifdef USE_PNP
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
#endif // USE_PNP        
    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_debug_handler, deviceTwinBinding)
{
    sensor_debug_enabled = *(bool*)deviceTwinBinding->propertyValue;
#ifdef USE_PNP
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
#else
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
#endif // USE_PNP

}
DX_DEVICE_TWIN_HANDLER_END


static void NetworkConnectionState(bool connected)
{
    static bool first_time = true;

    if (first_time && connected) {
        first_time = false;

        // This is the first connect so update device start time UTC and software version
        if (dx_isAzureConnected()) {

            dx_deviceTwinReportValue(&dt_version_string, "AvnetSK-V2-DevX");
            dx_deviceTwinReportValue(&dt_manufacturer, "Avnet");
            dx_deviceTwinReportValue(&dt_model, "Avnet Starter Kit");
        }
    }
}

// Read the current wifi configuration, output it to debug and send it up as device twin data
static void ReadWifiConfig(bool outputDebug){

    #define BSSID_SIZE 20
    char bssid[BSSID_SIZE];

	WifiConfig_ConnectedNetwork network;
	int result = WifiConfig_GetCurrentNetwork(&network);

	if (result < 0) 
	{
	    // Log_Debug("INFO: Not currently connected to a WiFi network.\n");
		strncpy(network_data.SSID, "Not Connected", 20);
		network_data.frequency_MHz = 0;
		network_data.rssi = 0;
	}
	else 
	{

        // Make sure we're connected to the IoTHub before updating the local variable or sending device twin updates
        if (dx_isAzureConnected()) {

            // Check to see if the SSID changed, if so update the SSID and send updated device twins properties
            if (strncmp(network_data.SSID, (char*)&network.ssid, network.ssidLength)!=0 ) {

                network_data.frequency_MHz = network.frequencyMHz;
                network_data.rssi = network.signalRssi;
                snprintf(bssid, BSSID_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x", network.bssid[0], network.bssid[1], network.bssid[2], network.bssid[3], network.bssid[4],
                         network.bssid[5]);

                // Clear the ssid array
                memset(network_data.SSID, 0, WIFICONFIG_SSID_MAX_LENGTH);
                strncpy(network_data.SSID, network.ssid, network.ssidLength);

#ifdef IOT_HUB_APPLICATION
                // Note that we send up this data to Azure if it changes, but the IoT Central Properties elements only 
                // show the data that was currenet when the device first connected to Azure.
                dx_deviceTwinReportValue(&dt_ssid, &network_data.SSID);
                dx_deviceTwinReportValue(&dt_freq, &network_data.frequency_MHz);
                dx_deviceTwinReportValue(&dt_bssid, &bssid);

#endif // IOT_HUB_APPLICATION

                if(outputDebug){

                    Log_Debug("SSID: %s\n", network_data.SSID);
                    Log_Debug("Frequency: %dMHz\n", network_data.frequency_MHz);
                    Log_Debug("bssid: %s\n", bssid);
                    Log_Debug("rssi: %d\n", network_data.rssi);
                }

                // Since we're connected to a wifi network and have reported the details to the IoTHub
                // modify the timer frequency from the default of 30 seconds to every 5 minutes
                dx_timerChange(&tmr_monitor_wifi_network, &(struct timespec){60*5, 0});
            }
        }
    }
}

/// <summary>
///  Function for rebootDevice directMethod
///  name: rebootDevice
///  Payload: {"delayTime": 0 < delay in seconds > 12*60*60}
///  Start Reboot Device Direct Method 'RebootDevice' {"delayTime":15}
/// </summary>
static DX_DIRECT_METHOD_HANDLER(dm_restart_device_handler, json, directMethodBinding, responseMsg)
{    
    char delay_str[] = "delayTime";
    int requested_delay_seconds;

    JSON_Object *jsonObject = json_value_get_object(json);
    if (jsonObject == NULL) {
        return DX_METHOD_FAILED;
    }

    // check JSON properties sent through are the correct type
    if (!json_object_has_value_of_type(jsonObject, delay_str, JSONNumber)) {
        return DX_METHOD_FAILED;
    }

    requested_delay_seconds = (int)json_object_get_number(jsonObject, delay_str);
    if (IN_RANGE(requested_delay_seconds, 1, (12*60*60))) {

        // Set the timer to fire after the requested delayTime
        dx_timerOneShotSet(&tmr_reboot, &(struct timespec){requested_delay_seconds, 0});
        return DX_METHOD_SUCCEEDED;
    
    }
    else{
        return DX_METHOD_FAILED;
    }
}
DX_DIRECT_METHOD_HANDLER_END

/// <summary>
///  Function for rebootDevice directMethod
///  name: haltApplication
///  Payload: None
/// </summary>
static DX_DIRECT_METHOD_HANDLER(dm_halt_device_handler, json, directMethodBinding, responseMsg)
{
    
    int requested_delay_seconds = HALT_APPLICATION_DELAY_TIME_SECONDS;

    // Set the timer to fire after the requested delayTime
    dx_timerOneShotSet(&tmr_reboot, &(struct timespec){requested_delay_seconds, 0});
    return DX_METHOD_SUCCEEDED;
}
DX_DIRECT_METHOD_HANDLER_END

/// <summary>
/// Restart the Device
/// </summary>
static DX_TIMER_HANDLER(delay_restart_timer_handler)
{
    PowerManagement_ForceSystemReboot();
}
DX_TIMER_HANDLER_END

/// </summary>
///  name: setSensor
///  payload: {"pollTime": 0 > integer < 12 hours >}
/// </summary>
static DX_DIRECT_METHOD_HANDLER(dm_set_sensor_poll_period, json, directMethodBinding, responseMsg)
{    
    char poll_str[] = "pollTime";
    int requested_poll_time_seconds;

    JSON_Object *jsonObject = json_value_get_object(json);
    if (jsonObject == NULL) {
        return DX_METHOD_FAILED;
    }

    // check JSON properties sent through are the correct type
    if (!json_object_has_value_of_type(jsonObject, poll_str, JSONNumber)) {
        return DX_METHOD_FAILED;
    }

    requested_poll_time_seconds = (int)json_object_get_number(jsonObject, poll_str);
    if (IN_RANGE(requested_poll_time_seconds, 1, (12*60*60))) {

        // Set the timer to fire after the requested delayTime
        dx_timerChange(&tmr_read_sensors, &(struct timespec){requested_poll_time_seconds, 0});
        return DX_METHOD_SUCCEEDED;
    
    }
    else{
        return DX_METHOD_FAILED;    
    }
}
DX_DIRECT_METHOD_HANDLER_END

/// <summary>
/// Handler to check for Button Presses
/// </summary>
static DX_TIMER_HANDLER(ButtonPressCheckHandler)
{
    // Assume the device comes up with the buttons at rest
    static GPIO_Value_Type buttonAState = GPIO_Value_High;
    static GPIO_Value_Type buttonBState = GPIO_Value_High;

    // Variables for the current state
    GPIO_Value_Type button_a_state;
    GPIO_Value_Type button_b_state;

    // Read both button states
    if(GPIO_GetValue(buttonA.fd, &button_a_state) < 0){
        Log_Debug("ERROR: buttonA read: errno=%d (%s)\n", errno, strerror(errno));
        dx_terminate(ExitCode_ReadButtonAError);
        return;
    }

    if(GPIO_GetValue(buttonB.fd, &button_b_state) < 0){
        Log_Debug("ERROR: buttonB read: errno=%d (%s)\n", errno, strerror(errno));
        dx_terminate(ExitCode_ReadButtonBError);
        return;
    }

    ProcessButtonState(button_a_state, &buttonAState, "buttonA");
    ProcessButtonState(button_b_state, &buttonBState, "buttonB");
}
DX_TIMER_HANDLER_END

static void ProcessButtonState(GPIO_Value_Type new_state, GPIO_Value_Type* old_state, const char* telemetry_key){

    // Did button state change?
    if(new_state != *old_state){

        // Update our local copy
        *old_state = new_state;

        // Check to see is the change is a button press
        if(new_state == GPIO_Value_Low){

#ifdef OLED_SD1306
            if(strncmp(telemetry_key, "buttonA", 7) == 0){
                // Use buttonA presses to drive OLED to display the previous screen     
                oled_state = (--oled_state < 0)? OLED_NUM_SCREEN : oled_state;
            }
            else{
                // Use buttonB presses to drive OLED to display the nexts screen     
                oled_state = (++oled_state > OLED_NUM_SCREEN)? 0 : oled_state;
            }
#endif // OLED_SD1306

        }
        // else the button was released

        // The button state changed, send up the current button state as telemetry
        if(sensor_debug_enabled){
            Log_Debug("%s %s!\n", telemetry_key, (new_state == GPIO_Value_Low) ? "Pressed": "Released");
        }
#ifdef IOT_HUB_APPLICATION
        SendButtonTelemetry(telemetry_key, new_state);
#endif // IOT_HUB_APPLICATION
    }
}

#ifdef IOT_HUB_APPLICATION
/// <summary>
/// Send button telemetry
/// </summary>
static void SendButtonTelemetry(const char* telemetry_key, GPIO_Value_Type button_state){

    // Serialize telemetry as JSON
    bool serialization_result = dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 1, 
        DX_JSON_INT, telemetry_key, (button_state == GPIO_Value_Low) ? 1: 0);

    if (serialization_result) {

        Log_Debug("%s\n", msgBuffer);
        dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);

    } else {
        Log_Debug("JSON Serialization failed\n");
    }
}
#endif // IOT_HUB_APPLICATION

#ifdef OLED_SD1306
static void UpdateOledEventHandler(EventLoopTimer *eventLoopTimer)
{

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        dx_terminate(ExitCode_ConsumeEventOledHandler);
        return;
    }

	// Update/refresh the OLED data
	update_oled();
}
#endif 

#ifdef M4_INTERCORE_COMMS
/// <summary>
/// alsPt19_receive_msg_handler()
/// This handler is called when the high level application receives a raw data read response from the 
/// AvnetAls-PT19 real time application.
/// </summary>
static void alsPt19_receive_msg_handler(void *data_block, ssize_t message_length)
{

// Cast the data block so we can index into the data
IC_COMMAND_BLOCK_ALS_PT19 *messageData = (IC_COMMAND_BLOCK_ALS_PT19*) data_block;

switch (messageData->cmd) {
    case IC_READ_SENSOR:
        // Pull the sensor data already in units of Lux
        light_sensor = (float)messageData->lightSensorLuxData;
        break;
    case IC_HEARTBEAT:
        Log_Debug("IC_HEARTBEAT\n");
        break;
    case IC_READ_SENSOR_RESPOND_WITH_TELEMETRY:
        Log_Debug("IC_READ_SENSOR_RESPOND_WITH_TELEMETRY\n");
//        Log_Debug("%s\n", messageData->telemetryJSON);

#ifdef IOT_HUB_APPLICATION
#ifdef USE_IOT_CONNECT
            // Add the IoTConnect metadata to the seralized telemetry
            dx_avnetJsonSerializePayload(messageData->telemetryJSON, avtMsgBuffer, sizeof(avtMsgBuffer), NULL);
            Log_Debug("%s\n", avtMsgBuffer);
            dx_azurePublish(avtMsgBuffer, strlen(avtMsgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);

#else // !USE_IOT_CONNECT
            Log_Debug("%s\n", messageData->telemetryJSON);
            dx_azurePublish(messageData->telemetryJSON, strlen(messageData->telemetryJSON), messageProperties, NELEMS(messageProperties), &contentProperties);
#endif // USE_IOT_CONNECT
#endif // IOT_HUB_APPLICATION

        break;
    case IC_SET_SAMPLE_RATE:
        Log_Debug("IC_SET_SAMPLE_RATE\n");
        break;
    case IC_UNKNOWN:
    default:
        break;
    }
}
#endif // M4_INTERCORE_COMMS
/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
#ifdef IOT_HUB_APPLICATION
#ifdef USE_IOT_CONNECT
    dx_avnetConnect(&dx_config, NETWORK_INTERFACE);
#else // not Avnet IoTConnect
    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
#endif // USE_IOT_CONNECT    
#endif // IOT_HUB_APPLICATION    
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));
    dx_azureRegisterConnectionChangedNotification(NetworkConnectionState);

    // Initialize the i2c sensors
    lp_imu_initialize();

#ifdef M4_INTERCORE_COMMS
    // Initialize Intercore Communications for core one
    if(!dx_intercoreConnect(&intercore_alsPt19_light_sensor)){
        dx_terminate(ExitCode_rtAppInitFailed);
    }
    else{
        RTCore_connected = true;
    }
#endif // M4_INTERCORE_COMMS
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
    lp_imu_close();

}

int main(int argc, char *argv[])
{
    dx_registerTerminationHandler();
    Log_Debug("Avnet Starter Kit Simple Reference Application starting.\n");

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
