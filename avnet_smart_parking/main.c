/* Copyright Avnet Inc. All rights reserved.
 * Licensed under the MIT License.
 ************************************************************************************************/

#include "main.h"

// Globals 
/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a raw data read response from the 
/// LightRanger5 real time application.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{
    // Cast the data block so we can index into the data
    IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_RT_TO_HL*) data_block;

    switch (messageData->cmd) {
        case IC_LIGHTRANGER5_CLICK_READ_SENSOR:

            // Pull the sensor data 
            currentRange = messageData->range_mm;

            // Determine what range we're in . . . 
            if(currentRange == -1){
                ledStatus = LED_RANGE_NO_TARGET;
            }
            else if((currentRange < MAXIUM_RANGE) && (currentRange >= midRange)){
                ledStatus = LED_RANGE_FAR;
            }
            else if((currentRange < midRange) && (currentRange >= closeRange)){
                ledStatus = LED_RANGE_MID;
            }
            else if((currentRange < closeRange) && (currentRange >= targetRange)){
                ledStatus = LED_RANGE_CLOSE;
            }
            else if(currentRange <= targetRange){
                ledStatus = LED_RANGE_STOP;
            }
            else{
                Log_Debug("no case for currentRange == %d\n", currentRange);
            }

            break;

        // Handle the other cases by doing nothing
        case IC_LIGHTRANGER5_CLICK_HEARTBEAT:
        case IC_LIGHTRANGER5_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY:
        case IC_LIGHTRANGER5_CLICK_SET_AUTO_TELEMETRY_RATE:
        case IC_LIGHTRANGER5_CLICK_UNKNOWN:
        default:
            break;
        }
}
static DX_TIMER_HANDLER(read_range_handler)
{
    //Code to read the sensor data in your application
    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_LIGHTRANGER5_CLICK_READ_SENSOR;
    dx_intercorePublish(&intercore_LIGHTRANGER5_click_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT));
}
DX_TIMER_HANDLER_END

static DX_TIMER_HANDLER(drive_leds_handler)
{
    static bool ledIsOn = true;
    int new_timer_period = -1;

    switch(ledStatus){
        case LED_RANGE_FAR: // Green LED flash
            dx_gpioOff(&userLedRed);
            dx_gpioOff(&userLedBlue);           

            if(ledIsOn){
                dx_gpioOff(&userLedGreen);
            }
            else{
                dx_gpioOn(&userLedGreen);
            }
            
            new_timer_period = LED_FLASH_RATE_FAR;
            break;
        case LED_RANGE_MID: // Blue LED flash

            dx_gpioOff(&userLedRed);
            dx_gpioOff(&userLedGreen);           

            if(ledIsOn){
                dx_gpioOff(&userLedBlue);
            }
            else{
                dx_gpioOn(&userLedBlue);
            }
            
            new_timer_period = LED_FLASH_RATE_MID;

            break;
        case LED_RANGE_CLOSE:  // Red LED flash
            dx_gpioOff(&userLedBlue);
            dx_gpioOff(&userLedGreen);           

            if(ledIsOn){
                dx_gpioOff(&userLedRed);
            }
            else{
                dx_gpioOn(&userLedRed);
            }
            
            new_timer_period = LED_FLASH_RATE_CLOSE;
            break;
            
        case LED_RANGE_STOP:  // Red LED on
            dx_gpioOn(&userLedRed);
            dx_gpioOff(&userLedGreen);
            dx_gpioOff(&userLedBlue);           
            new_timer_period = LED_FLASH_RATE_IDLE;
            break;

        case LED_RANGE_NO_TARGET: // Green LED on
        case LED_RANGE_UNDEFINED: // Green LED on
        default:
            dx_gpioOff(&userLedRed);
            dx_gpioOn(&userLedGreen);
            dx_gpioOff(&userLedBlue);
            new_timer_period = LED_FLASH_RATE_IDLE;
            break;
    }

    ledIsOn = !ledIsOn;
    dx_timerChange(&tmrDriveLeds, &(struct timespec){0, new_timer_period*ONE_MS});
}
DX_TIMER_HANDLER_END

/// <summary>
/// Handler to check for Button Presses
/// </summary>
static DX_TIMER_HANDLER(ButtonPressCheckHandler)
{
    static GPIO_Value_Type buttonAState;

    if (dx_gpioStateGet(&buttonA, &buttonAState)) {

        targetRange = currentRange;

        midRange   = targetRange + 2 * ((MAXIUM_RANGE - targetRange) / 3);
        closeRange = targetRange + 1 * ((MAXIUM_RANGE - targetRange) / 3);

//        Log_Debug("TargetRange = %dmm\n", targetRange);
//        Log_Debug("closeRange = %dmm\n", closeRange);
//        Log_Debug("midRange = %dmm\n", midRange);

        writeTargetRangeToMutableStorage(targetRange);
    }
}
DX_TIMER_HANDLER_END

bool initPersistantMemory(int *targetRangeMm){

    // Read the persistant memory, if the call returns false, then we 
    // do not have any data stored and we need to write the first record
    // Determine if the configuration has changed, if so update mutable storage
    if(!readTargetRangeFromMutableStorage(targetRangeMm)){

        Log_Debug("Creating Persistant file!!\n");

        // Write the default data to establish the "file"
        writeTargetRangeToMutableStorage(*targetRangeMm);
        return false;
    }
    else{
        Log_Debug("Persistant file exists!\n");        
        Log_Debug("TargetRange = %dmm\n", *targetRangeMm);
        midRange   = targetRange + 2 * ((MAXIUM_RANGE - targetRange) / 3);
        closeRange = targetRange + 1 * ((MAXIUM_RANGE - targetRange) / 3);
        Log_Debug("midRange = %dmm\n", midRange);
        Log_Debug("closeRange = %dmm\n", closeRange);
        writeTargetRangeToMutableStorage(targetRange);
        
        return true;
    }

}

bool readTargetRangeFromMutableStorage(int *targetRangeVar){

    int fd = Storage_OpenMutableFile();
    if (fd == -1) {
        return false;
    }

    int ret = read(fd, targetRangeVar, sizeof(*targetRangeVar));
    if (ret == -1) {
    }
    close(fd);

    if (ret < sizeof(*targetRangeVar)) {
        return false;
    }

    return true;
}

bool writeTargetRangeToMutableStorage(int targetRangeVar){
    
    int fd = Storage_OpenMutableFile();
    if(fd == -1){
        Log_Debug("Error opening mutable storage!\n");
        dx_terminate(APP_ExitCode_OpenMutableFileFailed);
        return false;
    }

    Log_Debug("Updating Mutable Storage!\n");

    // Write the data to persistant memory
    int ret = write(fd, &targetRangeVar, sizeof(targetRangeVar));
    
    if (ret == -1) {
        Log_Debug("ERROR: An error occurred while writing to mutable file\n");
        dx_terminate(APP_ExitCode_WriteFileWriteFailed);

    } else if (ret < sizeof(targetRangeVar)) {
        Log_Debug("ERROR: Only wrote %d of %d bytes requested\n", ret, (int)sizeof(targetRangeVar));
        dx_terminate(APP_ExitCode_WriteFileWriteFailed);
    }
    close(fd);
    return true;
}


/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
//    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));

    // Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
    dx_intercoreConnect(&intercore_LIGHTRANGER5_click_binding);
    initPersistantMemory(&targetRange);
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
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