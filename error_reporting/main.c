/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 *
 *   DISCLAIMER
 *
 *   The DevX library supports the Azure Sphere Developer Learning Path:
 *
 *	   1. are built from the Azure Sphere SDK Samples at
 *          https://github.com/Azure/azure-sphere-samples
 *	   2. are not intended as a substitute for understanding the Azure Sphere SDK Samples.
 *	   3. aim to follow best practices as demonstrated by the Azure Sphere SDK Samples.
 *	   4. are provided as is and as a convenience to aid the Azure Sphere Developer Learning
 *          experience.
 *
 *   DEVELOPER BOARD SELECTION
 *
 *   The following developer boards are supported.
 *
 *	   1. AVNET Azure Sphere Starter Kit.
 *     2. AVNET Azure Sphere Starter Kit Revision 2.
 *	   3. Seeed Studio Azure Sphere MT3620 Development Kit aka Reference Design Board or rdb.
 *	   4. Seeed Studio Seeed Studio MT3620 Mini Dev Board.
 *
 *   ENABLE YOUR DEVELOPER BOARD
 *
 *   Each Azure Sphere developer board manufacturer maps pins differently. You need to select the
 *      configuration that matches your board.
 *
 *   Follow these steps:
 *
 *	   1. Open CMakeLists.txt.
 *	   2. Uncomment the set command that matches your developer board.
 *	   3. Click File, then Save to save the CMakeLists.txt file which will auto generate the
 *          CMake Cache.
 * 
 ************************************************************************************************/

#include "hw/azure_sphere_learning_path.h" // Hardware definition

#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_direct_methods.h"
#include "dx_exit_codes.h"
#include "dx_gpio.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <applibs/log.h>
#include <applibs/powermanagement.h>
#include <signal.h>

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:labmonitor;1"
#define NETWORK_INTERFACE "wlan0"
#define ONE_MS 1000000

// Forward declarations
static DX_DIRECT_METHOD_RESPONSE_CODE ErrorReportHandler(
    JSON_Value *json, DX_DIRECT_METHOD_BINDING *directMethodBinding, char **responseMsg);

static void AzureConnectionCheckHandler(EventLoopTimer *eventLoopTimer);

// Variables
DX_USER_CONFIG dx_config;

/****************************************************************************************
 * GPIO Peripherals
 ****************************************************************************************/

static DX_GPIO_BINDING ledRed = {.pin = LED_RED,
                              .name = "ledRed",
                              .direction = DX_OUTPUT,
                              .initialState = GPIO_Value_Low,
                              .invertPin = true};

static DX_GPIO_BINDING ledBlue = {.pin = LED_BLUE,
                              .name = "ledBlue",
                              .direction = DX_OUTPUT,
                              .initialState = GPIO_Value_Low,
                              .invertPin = true};


// All GPIOs added to gpio_set will be opened in InitPeripheralsAndHandlers
DX_GPIO_BINDING *gpio_set[] = {&ledRed, &ledBlue};

/****************************************************************************************
 * Timer Bindings
 ****************************************************************************************/
static DX_TIMER_BINDING azureConnectionCeckTimer = {
    .period = {2, 0}, .name = "AzureConnectionCheckTimer", .handler = AzureConnectionCheckHandler};

// All timers referenced in timers with be opened in the InitPeripheralsAndHandlers function
DX_TIMER_BINDING *timers[] = {&azureConnectionCeckTimer};

/****************************************************************************************
 * Azure IoT Direct Method Bindings
 ****************************************************************************************/
static DX_DIRECT_METHOD_BINDING dm_error_report = {.methodName = "ErrorReport",
                                                    .handler = ErrorReportHandler};

// All direct methods referenced in direct_method_bindings will be subscribed to in
// the InitPeripheralsAndHandlers function
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {&dm_error_report};

/// <summary>
///  Initialize peripherals, device twins, direct methods, timers.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
    dx_timerSetStart(timers, NELEMS(timers));
    dx_gpioSetOpen(gpio_set, NELEMS(gpio_set));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timers, NELEMS(timers));
    dx_gpioSetClose(gpio_set, NELEMS(gpio_set));
    dx_directMethodUnsubscribe();
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
    return dx_getTerminationExitCode();
}

/****************************************************************************************
 * Implementation
 ****************************************************************************************/

// Direct method name = ErrorReport, json payload = {"ErrorCode": <integer>}
//  
//      {"ErrorCode": 0} // Generates SIGKILL
//      {"ErrorCode": 1} // Generates SIG
//      {"ErrorCode": x > 1} // Application exits and returns x to the OS 
//
static DX_DIRECT_METHOD_RESPONSE_CODE ErrorReportHandler(
    JSON_Value *json, DX_DIRECT_METHOD_BINDING *directMethodBinding, char **responseMsg)
{

    char errorCode_str[] = "ErrorCode";
    int errorCode = 0;

    JSON_Object *jsonObject = json_value_get_object(json);
    if (jsonObject == NULL) {
        return DX_METHOD_FAILED;
    }

    // check JSON properties sent through are the correct type
    if (!json_object_has_value_of_type(jsonObject, errorCode_str, JSONNumber)) {
        return DX_METHOD_FAILED;
    }

    errorCode = (int)json_object_get_number(jsonObject, errorCode_str);
    Log_Debug("ErrorCode %d \n", errorCode);

    // Turn off the LEDs before exiting
    dx_gpioOff(&ledRed);
    dx_gpioOff(&ledBlue);

    switch(errorCode)
    {
        case 0: // generate SIGKILL exit
            raise(SIGKILL);
            break;
        case 1: // generate SIGEGV exit
            raise(SIGSEGV);
            break;
        default: // exit the application and return the passed in value
            dx_terminate(errorCode);
            break;
    }
    return DX_METHOD_SUCCEEDED;
}

/// <summary>
/// Handler to check for Azure IoTHub connection status
/// </summary>
static void AzureConnectionCheckHandler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    if(dx_isAzureConnected()){
        dx_gpioOff(&ledRed);
        dx_gpioOn(&ledBlue);
    }
    else{
        dx_gpioOn(&ledRed);
        dx_gpioOff(&ledBlue);
    }
}
