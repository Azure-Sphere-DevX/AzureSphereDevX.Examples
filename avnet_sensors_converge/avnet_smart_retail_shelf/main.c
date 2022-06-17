/* Copyright (c) Avnet Incorporated. All rights reserved.
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
  ************************************************************************************************/
#include "main.h"

/****************************************************************************************
 * Implementation
 ****************************************************************************************/
void sendTelemetryBuffer(void){

    // Only send telemetry if we're connected to IoTConnect
    if (dx_isAvnetConnected()){

        Log_Debug("TX: %s\n", msgBuffer);
        dx_avnetPublish(msgBuffer, strnlen(msgBuffer, sizeof(msgBuffer)), NULL, 0, &contentProperties, NULL);
    }
}


bool readShelfHeight(int* shelf1Height, int* shelf2Height ){

    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_SMART_SHELF_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_SMART_SHELF_READ_SENSOR;

    // Intercore syncronise publish request then wait for read pattern with 1000 microsecond
    // timeout. Typical turn around time is 100 to 250 microseconds
    if (dx_intercorePublishThenRead(&intercore_smart_shelf_binding, &ic_tx_block, sizeof(IC_COMMAND_BLOCK_SMART_SHELF_HL_TO_RT)) < 0) {
        Log_Debug("Intercore message request/response failed\n");
    } else {

        // Cast the data block so we can index into the data
        IC_COMMAND_BLOCK_SMART_SHELF_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_SMART_SHELF_RT_TO_HL*)intercore_smart_shelf_binding.intercore_recv_block;

        // Verify we have received the expected response
        if(messageData->cmd == IC_SMART_SHELF_READ_SENSOR) {

            *shelf1Height = messageData->rangeShelf1_mm;
            *shelf2Height = messageData->rangeShelf2_mm;
            return true;

        }
    }
    return false;
}

// Direct method name = LightControl, json payload = {"State": true, "Duration":2} or {"State":
// false, "Duration":2}
static DX_DIRECT_METHOD_HANDLER(measureShelfHeightHandler, json, directMethodBinding, responseMsg)
{

    int emptyShelf1Height = -1;
    int emptyShelf2Height = -1;

    // Meaure shelfes empty depth
    if(readShelfHeight(&emptyShelf1Height, &emptyShelf2Height)){
    
        // Update the shelf structs
        productShelf1.shelfHeight_mm = emptyShelf1Height;
        productShelf2.shelfHeight_mm = emptyShelf2Height;

        // Write the new configuration to persistant memory
        updateConfigInMutableStorage(productShelf1, productShelf2, lowPowerEnabled, lowPowerSleepTime);

        Log_Debug("Shelf 1 height recorded in persistant memory as %d mm\n", productShelf1.shelfHeight_mm);
        Log_Debug("Shelf 2 height recorded in persistant memory as %d mm\n", productShelf2.shelfHeight_mm);

        // Report the new shelf depths to IoTConnect
        dx_deviceTwinReportValue(&dt_measured_shelf1_height, &productShelf1.shelfHeight_mm);
        dx_deviceTwinReportValue(&dt_measured_shelf2_height, &productShelf2.shelfHeight_mm);
    }

    return DX_METHOD_SUCCEEDED;
}
DX_DIRECT_METHOD_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_shelf_time_handler, deviceTwinBinding)
{
    // validate data is sensible range before applying
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_INT &&
        *(int *)deviceTwinBinding->propertyValue >= MIN_SHELF_TIME &&
        *(int *)deviceTwinBinding->propertyValue <= MAX_SHELF_TIME) {

            min_shelf_time_for_detection = *(int*)deviceTwinBinding->propertyValue;
            Log_Debug("New minimum shelf time: %d seconds\n", min_shelf_time_for_detection);
    
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);   
    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_simulate_shelf_data_handler, deviceTwinBinding)
{

    // validate data is sensible range before applying
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_BOOL) {

        Log_Debug("Simulated Shelf Data %s\n", *(bool*)deviceTwinBinding->propertyValue ? "Enabled" : "Disabled");

        // Reset the last count so we'll send new shelf data
        productShelf1.lastProductCount = -1;
        productShelf2.lastProductCount = -1;

        // reset inter-core block
        memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_SMART_SHELF_HL_TO_RT));

        // Send read sensor message to realtime core app one
        ic_tx_block.cmd = IC_SMART_SHELF_SIMULATE_DATA;
        ic_tx_block.simulateShelfData = *(bool*)deviceTwinBinding->propertyValue;

        // Intercore syncronise publish request then wait for read pattern with 1000 microsecond
        // timeout. Typical turn around time is 100 to 250 microseconds
        if (dx_intercorePublishThenRead(&intercore_smart_shelf_binding, &ic_tx_block, sizeof(IC_COMMAND_BLOCK_SMART_SHELF_HL_TO_RT)) < 0) {
            Log_Debug("Intercore message request/response failed\n");
        } else {

            // Cast the data block so we can index into the data
            IC_COMMAND_BLOCK_SMART_SHELF_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_SMART_SHELF_RT_TO_HL*)intercore_smart_shelf_binding.intercore_recv_block;

            // Verify we have received the expected response
            if(messageData->cmd == IC_SMART_SHELF_SIMULATE_DATA) {

                Log_Debug("Simulated Shelf Data set to %s\n", messageData->simulateShelfData ? "Enabled" : "Disabled");
            }
        }

        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
    }
}
DX_DEVICE_TWIN_HANDLER_END


static DX_DEVICE_TWIN_HANDLER(dt_low_power_mode_handler, deviceTwinBinding)
{
    // validate data is sensible range before applying
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_BOOL) {

        Log_Debug("Low Power Mode %s\n", *(bool*)deviceTwinBinding->propertyValue ? "Enabled" : "Disabled");
        // BW implement low power shutdown

        lowPowerEnabled = *(bool*)deviceTwinBinding->propertyValue;

        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
        updateConfigInMutableStorage(productShelf1, productShelf2, lowPowerEnabled, lowPowerSleepTime);
    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_low_power_sleep_period_handler, deviceTwinBinding)
{
    // validate data is sensible range before applying
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_INT &&
        *(int *)deviceTwinBinding->propertyValue >= MIN_SLEEP_PERIOD &&
        *(int *)deviceTwinBinding->propertyValue <= MAX_SLEEP_PERIOD) {

            lowPowerSleepTime = *(int*)deviceTwinBinding->propertyValue;
            Log_Debug("New low power sleep time: %d seconds\n", lowPowerSleepTime);
    
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);   
        updateConfigInMutableStorage(productShelf1, productShelf2, lowPowerEnabled, lowPowerSleepTime);
    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_product_height_handler, deviceTwinBinding)
{
    productShelf_t* shelfPtr = NULL;

    // validate data is sensible range before applying
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_INT &&
        *(int *)deviceTwinBinding->propertyValue >= MIN_PRODUCT_HEIGHT_MM &&
        *(int *)deviceTwinBinding->propertyValue <= MAX_PRODUCT_HEIGHT_MM) {

        //  Validate the context pointer is not NULL
        if(deviceTwinBinding->context != NULL ){

            // Update the structure with the new value
            shelfPtr = (productShelf_t*) deviceTwinBinding->context;
            shelfPtr->productHeight_mm = *(int*)deviceTwinBinding->propertyValue;
            Log_Debug("%s, product height: %dmm\n", shelfPtr->name, shelfPtr->productHeight_mm);
        }

        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
        updateConfigInMutableStorage(productShelf1, productShelf2, lowPowerEnabled, lowPowerSleepTime);
    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_product_reserve_handler, deviceTwinBinding)
{
    productShelf_t* shelfPtr = NULL;

    // validate data is sensible range before applying
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_INT &&
        *(int *)deviceTwinBinding->propertyValue >= MIN_PRODUCT_RESERVE &&
        *(int *)deviceTwinBinding->propertyValue <= MAX_PRODUCT_RESERVE) {

        //  Validate the context pointer is not NULL
        if(deviceTwinBinding->context != NULL ){

            // Update the structure with the new value
            shelfPtr = (productShelf_t*) deviceTwinBinding->context;
            shelfPtr->productReserve = *(int*)deviceTwinBinding->propertyValue;;
            Log_Debug("%s, product reserve set to %d units\n", shelfPtr->name, shelfPtr->productReserve);
        }
    
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);    
        updateConfigInMutableStorage(productShelf1, productShelf2, lowPowerEnabled, lowPowerSleepTime);
    }        
}
DX_DEVICE_TWIN_HANDLER_END

void printConfig(void)
{
    Log_Debug("Low Power Settings:\n");
    Log_Debug("  Low Power Mode %s\n", lowPowerEnabled ? "Enabled": "Disabled");
    Log_Debug("  Low Power Sleep Period: %d Seconds\n\n", lowPowerSleepTime);

    Log_Debug("Shelf #1 Config:\n");
    Log_Debug(" Product Height: %d mm\n", productShelf1.productHeight_mm);
    Log_Debug(" Product Reserve: %d units\n", productShelf1.productReserve);
    Log_Debug(" Shelf Height: %d mm\n\n", productShelf1.shelfHeight_mm);
        
    Log_Debug("Shelf #2 Config:\n");
    Log_Debug(" Product Height: %d mm\n", productShelf2.productHeight_mm);
    Log_Debug(" Product Reserve: %d units\n", productShelf2.productReserve);
    Log_Debug(" Shelf Height: %d mm\n\n", productShelf2.shelfHeight_mm);
}

/// <summary>
/// Handler to check for Button Presses
/// </summary>
static DX_TIMER_HANDLER(ButtonPressCheckHandler)
{
    // Assume the device comes up with the buttons at rest
    static GPIO_Value_Type buttonAState = GPIO_Value_High;
    static GPIO_Value_Type buttonBState = GPIO_Value_High;

    if(dx_gpioStateGet(&buttonA, &buttonAState)){

        // Turn on the App LED to indicate that we're measureing and updating the persistant config        
        dx_gpioOn(&wifiLed);
        sleep(1);
        dx_gpioOff(&wifiLed);
        return;
    }

    if(dx_gpioStateGet(&buttonB, &buttonBState)){
    
        dx_gpioOn(&appLed);
        sleep(1);
        dx_gpioOff(&appLed);
        return;
    }
}
DX_TIMER_HANDLER_END

/// <summary>
/// Send message to realtime core app.
/// The response will be synchronous, the high-level app with wait on read until a
/// message is received from the real-time core or the read times out
/// </summary>
static DX_TIMER_HANDLER(read_sensors_handler)
{

    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_SMART_SHELF_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_SMART_SHELF_READ_SENSOR;

    // Intercore syncronise publish request then wait for read pattern with 1000 microsecond
    // timeout. Typical turn around time is 100 to 250 microseconds
    if (dx_intercorePublishThenRead(&intercore_smart_shelf_binding, &ic_tx_block, sizeof(IC_COMMAND_BLOCK_SMART_SHELF_HL_TO_RT)) < 0) {
        Log_Debug("Intercore message request/response failed\n");
    } else {

        // Cast the data block so we can index into the data
        IC_COMMAND_BLOCK_SMART_SHELF_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_SMART_SHELF_RT_TO_HL*)intercore_smart_shelf_binding.intercore_recv_block;

        // Verify we have received the expected response
        if(messageData->cmd == IC_SMART_SHELF_READ_SENSOR) {

            // Update the PHT data
            pht.pressure = messageData->pressure;
            pht.hum = messageData->hum;
            pht.temp = messageData->temp;

            processPeopleData(messageData->rangePeople_mm);

            // Update the shelf data
            calculateStockLevel(&productShelf1, messageData->rangeShelf1_mm);
            calculateStockLevel(&productShelf2, messageData->rangeShelf2_mm);
        }
    }
}
DX_TIMER_HANDLER_END

static DX_TIMER_HANDLER(send_telemetry_handler)
{

    // Send PHT telemetry.  The PHT sensor data is updated every 1 seconds.  Pull the most
    // recent data and send it up to the IoTConnect solution.
        
    // Serialize telemetry as JSON
    bool serialization_result = dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 3, 
        DX_JSON_FLOAT, "pressure", pht.pressure, 
        DX_JSON_FLOAT, "humidity", pht.hum, 
        DX_JSON_FLOAT, "temperature", pht.temp);

    if (serialization_result) {    
        sendTelemetryBuffer();
    }

    // Restart the one shot timer using the desred telemetry period from IoTConnect
    dx_timerOneShotSet(&tmr_send_telemetry, &(struct timespec){ dx_avnetGetDataFrequency(), 0});

}
DX_TIMER_HANDLER_END

// This handler examines the current stock levels for both shelfs and if there is a change the logic . . .
// 1. Verifies that the shelf data is consistant for at least 5 entries into the handler
// 2. Sends the new stock level up to IoTConnect as telemetry
// 3. If the new stock level is below the reserve stock level sends up telemetry showing the low stock alert
static DX_TIMER_HANDLER(shelf_stock_check_handler)
{

    // Only check stock if we're connected to IoTConnect
    if(dx_isAvnetConnected()){

        static stockHistory_t historyShelf1 = {.historyIndex = 0,
                                            .historyArray = {-1, -2, -3, -4}};

        static stockHistory_t historyShelf2 = {.historyIndex = 0,
                                            .historyArray = {-1, -2, -3, -4}};

        checkShelfStock(&productShelf1, &historyShelf1);
        checkShelfStock(&productShelf2, &historyShelf2);
    }
}
DX_TIMER_HANDLER_END

static void processPeopleData(int rangePeople_mm){

    static int startTimeSec = -1;
    static bool measuringAttentionTime = false;
    int totalShelfAttentionTime = 0;

    // Someone is standing in front of the shelf
    if((rangePeople_mm > 0) && (!measuringAttentionTime)){


        struct timespec now = {0, 0};
        clock_gettime(CLOCK_MONOTONIC, &now);

        // Did someone just walk in front of the shelf?
        startTimeSec = now.tv_sec;
        measuringAttentionTime = true;
    }
    
    // Someone was in front of the shelf and they left
    else if ((rangePeople_mm < 0) && (measuringAttentionTime)){

        // If the flag is true, then a person was in front of the 
        // shelf, and has just left.  Capture the totel time
        
        struct timespec now = {0, 0};
        clock_gettime(CLOCK_MONOTONIC, &now);
        int timeNowSec = now.tv_sec;

        totalShelfAttentionTime = timeNowSec - startTimeSec;
        measuringAttentionTime = false;
        if(totalShelfAttentionTime >= MINIMUM_SHELF_TIME){

            // Serialize telemetry as JSON
            bool serialization_result = dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 1, 
                DX_JSON_INT, "shelfTime", totalShelfAttentionTime);

            if (serialization_result) {    
                sendTelemetryBuffer();
            }
        }
    }
}

static int calculateStockLevel(productShelf_t* shelf, int range_mm){

    // Validate that all data is reasonable before doing any math
    if((shelf->shelfHeight_mm < 0       ) ||
       (shelf->productHeight_mm < 0     ) ||
       (range_mm < 0                    ) ||
       (range_mm > shelf->shelfHeight_mm)){
           
           return 0; 
    }

    shelf->currentProductCount = (shelf->shelfHeight_mm - range_mm) / (shelf->productHeight_mm + 3);
    return (shelf->currentProductCount);
}

bool checkShelfStock(productShelf_t* shelfData, stockHistory_t* shelfHistory){
    
    bool sendStockAlert = false;
    bool lowStock;

    // Examine shelf data to see if the stock level changed
    if(shelfData->currentProductCount != shelfData->lastProductCount){

        // Copy the latest product count value into the next items in the history array
        shelfHistory->historyArray[shelfHistory->historyIndex++] = shelfData->currentProductCount;
        shelfHistory->historyIndex &= STOCK_HISTORY_DEPTH_MASK;

        // Check to see if all elemements in the array are the same as the current product count
        if(shelfHistory->historyArray[0] == shelfData->currentProductCount &&
           shelfHistory->historyArray[1] == shelfData->currentProductCount &&
           shelfHistory->historyArray[2] == shelfData->currentProductCount &&
           shelfHistory->historyArray[3] == shelfData->currentProductCount){

            //Log_Debug("Stock on %s changed from %d to %d\n", shelfData->name, shelfData->lastProductCount, shelfData->currentProductCount);
            
            // Check to see if we added enough stock to get at or above the reserve level
            if((shelfData->currentProductCount >= shelfData->productReserve) &&
               (shelfData->lastProductCount < shelfData->productReserve)){
                    
                // Send a clear low stock alert
                sendStockAlert = true;
                lowStock = false;

            } // Check to see if we are below the low stock level
            else if((shelfData->currentProductCount < shelfData->productReserve) &&
                    (shelfData->lastProductCount >= shelfData->productReserve)){

                // Send a low stock alert
                sendStockAlert = true;
                lowStock = true;
            }
            
            if(sendStockAlert){

                // Serialize telemetry as JSON
                bool serialization_result = dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 1, 
                DX_JSON_BOOL, shelfData->alertName, lowStock);

                if (serialization_result) {

                    if (serialization_result) {    
                        sendTelemetryBuffer();
                    }
                }
            }            

            // Update the data structure with the new product count
            shelfData->lastProductCount = shelfData->currentProductCount;

            // Serialize telemetry as JSON
            bool serialization_result = dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 1, 
            DX_JSON_INT, shelfData->name, shelfData->currentProductCount);

            if (serialization_result) {

                if (serialization_result) {    
                    sendTelemetryBuffer();
                }
            }
            // Inform the calling routine that the stock level changed.
            return true;
        }
    }   
    return false;
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void){
#ifdef USE_AVNET_IOTCONNECT
    dx_avnetSetApiVersion(AVT_API_VERSION_2_1);
    //dx_avnetSetDebugLevel(AVT_DEBUG_LEVEL_VERBOSE);
    dx_avnetConnect(&dx_config, NETWORK_INTERFACE);
#else     
//    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
#endif     
    
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_intercoreConnect(&intercore_smart_shelf_binding);
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));

    // The persistant memory logic will always try to read the existing configuration from mutable storage before
    // making any updates.  If the application is running for the very first time, the record will not exist.
    // The call to initPersistantMemory() will check for this condition and write an initial record if needed.
    if(initPersistantMemory(productShelf1, productShelf2, lowPowerEnabled, lowPowerSleepTime)){
        update_config_from_mutable_storage(&productShelf1, &productShelf2, &lowPowerEnabled, &lowPowerSleepTime);
        printConfig();
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