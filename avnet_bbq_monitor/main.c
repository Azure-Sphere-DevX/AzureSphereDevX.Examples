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

// Forwared declaration
float calculateTargetTemp(void);

/****************************************************************************************
 * Globals with defaults
 ****************************************************************************************/
static meat_t meatType = STEAK;
static steak_order_t steakOrderTemp = MEDIUM_RARE;
static int targetTempPolo = 165;
static int targetTempSwine = 145;
static int overDoneDelta = 5;

/****************************************************************************************
 * Implementation
 ****************************************************************************************/
static void setDinnerStatusLed(Dinner_Status dinnerStatus)
{

    // Define an array of the led bindings so we can process the loop
    DX_GPIO_BINDING *dinnerLEDs[] = {&red_led, &green_led, &blue_led};

    // Turn off all the LEDs before setting the new state
    dx_gpioOff(&red_led);
    dx_gpioOff(&green_led);
    dx_gpioOff(&blue_led);

    // Loop through the dinnerStatus bits
    for (int8_t bitIndex = 0; bitIndex < NELEMS(dinnerLEDs); bitIndex++) {
        if ((int8_t)dinnerStatus & (1 << bitIndex)) {
            dx_gpioOn(dinnerLEDs[bitIndex]);
        }
    }
}

static void buzz_click_alarm(bool alarmOn, DX_PWM_BINDING *pwmDevice)
{

    if (alarmOn) {

        dx_pwmSetDutyCycle(&pwm_buzz_click, 5000, 1);

    } else {

        dx_pwmStop(&pwm_buzz_click);
    }
}

/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a raw data read response from the
/// Thermo CLICK real time application.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{
    float currentTempF = 0.0;
    float currentTargetTemp = 0.0;

    // Cast the data block so we can index into the data
    IC_COMMAND_BLOCK_THERMO_CLICK_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_THERMO_CLICK_RT_TO_HL *)data_block;

    switch (messageData->cmd) {
        case IC_THERMO_CLICK_READ_SENSOR:
            // Pull the sensor data
            currentTempF = (messageData->temperature * 9.0F / 5.0F) + 32.0F;
            //Log_Debug("IC_THERMO_CLICK_READ_SENSOR: tempC: %.2f\n", messageData->temperature);
            //Log_Debug("IC_THERMO_CLICK_READ_SENSOR: tempF: %.2f\n", currentTempF);

            // Drive the user interfaces based on the target meat temp and the current temp
            currentTargetTemp = calculateTargetTemp();

            // Dinner is still cooking
            if (currentTempF < currentTargetTemp) {

                setDinnerStatusLed(RGB_UNDER_TARGET_TEMP);
                buzz_click_alarm(false, &pwm_buzz_click);
            }

            // Dinner is overdone!
            else if (currentTempF > (currentTargetTemp + (float)overDoneDelta)) {

                setDinnerStatusLed(RGB_OVER_DONE);
                buzz_click_alarm(true, &pwm_buzz_click);

                // Dinner is overdone!
            } else if (currentTempF > currentTargetTemp) {

                setDinnerStatusLed(RGB_DINNER_READY);
                buzz_click_alarm(true, &pwm_buzz_click);

            } else {
                Log_Debug("ERROR: temperature logic is broken\n");
            }

            // Send the current data to the OLED
            oled_update(&oled_i2c, currentTargetTemp, currentTempF, SHOW_BBQ_STATUS);

        // Handle the other cases by doing nothing
        case IC_THERMO_CLICK_HEARTBEAT:
        case IC_THERMO_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY:
        case IC_THERMO_CLICK_SET_AUTO_TELEMETRY_RATE:
        case IC_THERMO_CLICK_UNKNOWN:
        default:
            break;
    }
}

/// <summary>
/// Periodic timer handler example
/// </summary>
static void read_and_process_sensor_data_handler(EventLoopTimer *eventLoopTimer)
{

    static Dinner_Status currentStatus = RGB_ALL_LEDS;
    Log_Debug("currentStatus: 0X%x\n", currentStatus);

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    // Request the temperature data from the real-time application
    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_THERMO_CLICK_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_THERMO_CLICK_READ_SENSOR;
    dx_intercorePublish(&intercore_thermo_click_binding, &ic_tx_block, sizeof(IC_COMMAND_BLOCK_THERMO_CLICK_HL_TO_RT));
}

static void dt_target_meat_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    int tempMeatType = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (IN_RANGE(tempMeatType, STEAK, SWINE)) {

        meatType = tempMeatType;
        Log_Debug("New target meat: %d\n", meatType);

        // Ack the reported value back to IoTCentral
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    } else {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, &meatType, DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}

static void dt_target_temp_steak_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    int tempSteakOrderTemp = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (IN_RANGE(tempSteakOrderTemp, RARE, WELL_DONE)) {

        steakOrderTemp = tempSteakOrderTemp;
        Log_Debug("New steak order temp: %d\n", steakOrderTemp);

        // Ack the reported value back to IoTCentral
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    } else {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, &steakOrderTemp, DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}

static void dt_target_temp_polo_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    int tempTargetTempPolo = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (IN_RANGE(tempTargetTempPolo, 165, 180)) {

        targetTempPolo = tempTargetTempPolo;
        Log_Debug("New target temp Polo: %d\n", targetTempPolo);

        // Ack the reported value back to IoTCentral
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    } else {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, &targetTempPolo, DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}

static void dt_target_temp_swine_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    int tempTargetTempSwine = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (IN_RANGE(tempTargetTempSwine, 145, 160)) {

        targetTempSwine = tempTargetTempSwine;
        Log_Debug("New target temp Swine: %d\n", targetTempSwine);

        // Ack the reported value back to IoTCentral
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    } else {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, &targetTempSwine, DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}

static void dt_temp_over_done_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    int tempOverDoneDelta = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (IN_RANGE(tempOverDoneDelta, 1, 20)) {

        overDoneDelta = tempOverDoneDelta;
        Log_Debug("New over done delta: %d\n", overDoneDelta);

        // Ack the reported value back to IoTCentral
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    } else {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, &overDoneDelta, DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}

// Use the device twin data to determine the current target temperature
float calculateTargetTemp(void)
{

    // Use the device twin data to determine the target temp for the meat on the BBQ

    switch (meatType) {
    case POLO:
        return (float)targetTempPolo;
        break;
    case SWINE:
        return (float)targetTempSwine;
        break;
    case STEAK:
        return (float)steakOrderTemp;
        break;
    default:
        return NAN;
        break;
    }
}

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

    // Open the PWM interface
    dx_pwmSetOpen(pwm_bindings, NELEMS(pwm_bindings));

    // Turn off the Buzz CLICK
    dx_pwmStop(&pwm_buzz_click);

    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));

    dx_intercoreConnect(&intercore_thermo_click_binding);

    if (dx_i2cOpen(&oled_i2c)) {
        oled_init(&oled_i2c);
        oled_update(&oled_i2c, 0.0, 0.0, SHOW_LOGO);
    }

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
    dx_pwmSetClose(pwm_bindings, NELEMS(pwm_bindings));
    dx_i2cClose(&oled_i2c);
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