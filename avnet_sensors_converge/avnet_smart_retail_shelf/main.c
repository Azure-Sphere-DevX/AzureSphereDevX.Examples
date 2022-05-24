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
static DX_DEVICE_TWIN_HANDLER(dt_low_power_mode_handler, deviceTwinBinding)
{
    // validate data is sensible range before applying
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_BOOL) {

        Log_Debug("Low Power Mode %s\n", *(bool*)deviceTwinBinding->propertyValue ? "enabled" : "Disabled");
        // BW implement low power shutdown

        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
        updateConfigInMutableStorage();
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
        updateConfigInMutableStorage();
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
        updateConfigInMutableStorage();
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
        updateConfigInMutableStorage();
    }        
}
DX_DEVICE_TWIN_HANDLER_END

bool read_config_from_mutable_storage(persistantMemory_t* persistantConfig){

    int fd = Storage_OpenMutableFile();
    if (fd == -1) {
        Log_Debug("ERROR: Could not open mutable file!\n");
        dx_terminate(APP_ExitCode_OpenMutableFileFailed);
        return false;
    }

    int ret = read(fd, persistantConfig, sizeof(*persistantConfig));
    if (ret == -1) {
        Log_Debug("ERROR: An error occurred while reading file!\n");
        dx_terminate(APP_ExitCode_ReadFile_ReadFailed);
    }
    close(fd);

    if (ret < sizeof(*persistantConfig)) {
        return false;
    }

    return true;
}

bool write_config_to_mutable_storage(void){
    
    int fd = Storage_OpenMutableFile();
    if(fd == -1){
        Log_Debug("Error opening mutable storage!\n");
        dx_terminate(APP_ExitCode_OpenMutableFileFailed);
        return false;
    }

    Log_Debug("Updating Mutable Storage!\n");

    // Copy all the current config data for both shelfs and low power mode into the 
    // local structure

    persistantMemory_t persistantConfig;

    // Low power Config
    persistantConfig.sleepTime = lowPowerSleepTime;
    persistantConfig.lowPowerModeEnabled = lowPowerEnabled;
    
    // Shelf #1 Config
    persistantConfig.shelf1Copy.productHeight_mm = productShelf1.productHeight_mm;
    persistantConfig.shelf1Copy.productReserve = productShelf1.productReserve;
    persistantConfig.shelf1Copy.shelfHeight_mm = productShelf1.shelfHeight_mm;

    // Shelf #2 Config
    persistantConfig.shelf2Copy.productHeight_mm = productShelf2.productHeight_mm;
    persistantConfig.shelf2Copy.productReserve = productShelf2.productReserve;
    persistantConfig.shelf2Copy.shelfHeight_mm = productShelf2.shelfHeight_mm;

    // Write the global structure to persistant memory
    int ret = write(fd, &persistantConfig, sizeof(persistantConfig));
    
    if (ret == -1) {
        Log_Debug("ERROR: An error occurred while writing to mutable file\n");
        dx_terminate(APP_ExitCode_WriteFileWriteFailed);

    } else if (ret < sizeof(persistantConfig)) {
        Log_Debug("ERROR: Only wrote %d of %d bytes requested\n", ret, (int)sizeof(persistantConfig));
        dx_terminate(APP_ExitCode_WriteFileWriteFailed);
    }
    close(fd);
    return true;
}

bool updateConfigInMutableStorage(void){

    // Determine if the configuration has changed, if so update mutable storage
    persistantMemory_t localConfigCopy;
    if(read_config_from_mutable_storage(&localConfigCopy)){

        // Start to check each configuration item, if we find one that's been updated,
        // then write the new config and exit

        // Low power mode config
        if(localConfigCopy.lowPowerModeEnabled != lowPowerEnabled){
            write_config_to_mutable_storage();
            return true;
        }

        if(localConfigCopy.sleepTime != lowPowerSleepTime){
            write_config_to_mutable_storage();
            return true;
        }

        // Shelf #1 config
        if(localConfigCopy.shelf1Copy.productHeight_mm != productShelf1.productHeight_mm){
            write_config_to_mutable_storage();
            return true;
        }

        if(localConfigCopy.shelf1Copy.productReserve != productShelf1.productReserve){
            write_config_to_mutable_storage();
            return true;
        }

        if(localConfigCopy.shelf1Copy.shelfHeight_mm != productShelf1.shelfHeight_mm){
            write_config_to_mutable_storage();
            return true;
        }

        // Shelf #1 config
        if(localConfigCopy.shelf2Copy.productHeight_mm != productShelf2.productHeight_mm){
            write_config_to_mutable_storage();
            return true;
        }

        if(localConfigCopy.shelf2Copy.productReserve != productShelf2.productReserve){
            write_config_to_mutable_storage();
            return true;
        }

        if(localConfigCopy.shelf2Copy.shelfHeight_mm != productShelf2.shelfHeight_mm){
            write_config_to_mutable_storage();
            return true;
        }
    }

    Log_Debug("Config not updated, no changes detected\n");
    return false;
}

void update_config_from_mutable_storage(void){

    int fd = Storage_OpenMutableFile();
    if(fd == -1){
        Log_Debug("Error opening mutable storage!\n");
        dx_terminate(APP_ExitCode_OpenMutableFileFailed);
    }

    Log_Debug("Updating Config from Mutable Storage!\n");

    // Read the persistant config
    persistantMemory_t localConfigCopy;
    if(read_config_from_mutable_storage(&localConfigCopy)){

        // Low power Config
        lowPowerSleepTime = localConfigCopy.sleepTime;
        lowPowerEnabled = localConfigCopy.lowPowerModeEnabled;
        
        // Shelf #1 Config
        productShelf1.productHeight_mm = localConfigCopy.shelf1Copy.productHeight_mm;
        productShelf1.productReserve = localConfigCopy.shelf1Copy.productReserve;
        productShelf1.shelfHeight_mm = localConfigCopy.shelf1Copy.shelfHeight_mm;

        // Shelf #2 Config
        productShelf2.productHeight_mm = localConfigCopy.shelf2Copy.productHeight_mm;
        productShelf2.productReserve = localConfigCopy.shelf2Copy.productReserve;
        productShelf2.shelfHeight_mm = localConfigCopy.shelf2Copy.shelfHeight_mm;

        close(fd);
    }
}

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
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
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
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));

    printConfig();
    update_config_from_mutable_storage();
    printConfig();

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