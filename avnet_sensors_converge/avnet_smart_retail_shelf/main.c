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

    // Meaure shelf #1 empty depth
    if(dx_gpioStateGet(&buttonA, &buttonAState)){

        // Turn on the App LED to indicate that we're measureing and updating the persistant config        
        dx_gpioOn(&wifiLed);
        productShelf1.shelfHeight_mm = 200;
        updateConfigInMutableStorage(productShelf1, productShelf2, lowPowerEnabled, lowPowerSleepTime);
        Log_Debug("Shelf 1 height recorded in persistant memory as %d mm\n", productShelf1.shelfHeight_mm);
        sleep(1);
        dx_gpioOff(&wifiLed);
        return;
    }

    // Meaure shelf #1 empty depth
    if(dx_gpioStateGet(&buttonB, &buttonBState)){
    
        dx_gpioOn(&appLed);
        productShelf2.shelfHeight_mm = 100;
        updateConfigInMutableStorage(productShelf1, productShelf2, lowPowerEnabled, lowPowerSleepTime);
        Log_Debug("Shelf 2 height recorded in persistant memory as %d mm\n", productShelf2.shelfHeight_mm);
        sleep(1);
        dx_gpioOff(&appLed);
        return;
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
    //dx_avnetSetDebugLevel(AVT_DEBUG_LEVEL_VERBOSE);
    dx_avnetConnect(&dx_config, NETWORK_INTERFACE);
#else     
//    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
#endif     
    
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));

    update_config_from_mutable_storage(&productShelf1, &productShelf2, &lowPowerEnabled, &lowPowerSleepTime);
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