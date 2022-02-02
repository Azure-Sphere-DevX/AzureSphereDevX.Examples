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
 ************************************************************************************************/

#include "main.h"

static DX_TIMER_HANDLER(publish_message_handler)
{
    static int msgId = 0;

    if (azure_connected && environment.validated)
    {
        // clang-format off
        // Serialize environment as JSON
        bool serialization_result = dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 4,
            DX_JSON_INT, "msgId", msgId++,
            DX_JSON_INT, "temperature", environment.latest.temperature,
            DX_JSON_INT, "humidity", environment.latest.humidity,
            DX_JSON_INT, "pressure", environment.latest.pressure);
        // clang-format on

        if (serialization_result)
        {
            Log_Debug("%s\n", msgBuffer);
            dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);
        }
        else
        {
            dx_terminate(APP_ExitCode_Telemetry_Buffer_Too_Small);
        }
    }
}
DX_TIMER_HANDLER_END

/// <summary>
///  Generate some fake sensor data
/// </summary>
DX_TIMER_HANDLER(read_sensor_handler)
{
    environment.latest.temperature = 25;
    environment.latest.humidity = 55;
    environment.latest.pressure = 1050;
    environment.validated = true;
}
DX_TIMER_HANDLER_END

/// <summary>
/// Determine if environment value changed. If so, update it's device twin
/// </summary>
/// <param name="new_value"></param>
/// <param name="previous_value"></param>
/// <param name="device_twin"></param>
static void device_twin_update(int *latest_value, int *previous_value, DX_DEVICE_TWIN_BINDING *device_twin)
{
    if (*latest_value != *previous_value)
    {
        *previous_value = *latest_value;
        dx_deviceTwinReportValue(device_twin, latest_value);
    }
}

static DX_TIMER_HANDLER(report_properties_handler)
{
    if (azure_connected && environment.validated)
    {
        device_twin_update(&environment.latest.temperature, &environment.previous.temperature, &dt_temperature);
        device_twin_update(&environment.latest.pressure, &environment.previous.pressure, &dt_pressure);
        device_twin_update(&environment.latest.humidity, &environment.previous.humidity, &dt_humidity);
    }
}
DX_TIMER_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_desired_sample_rate_handler, deviceTwinBinding)
{
    int sample_rate_seconds = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (IN_RANGE(sample_rate_seconds, 1, 120))
    {
        dx_timerChange(&tmr_read_sensor, &(struct timespec){sample_rate_seconds, 0});
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    }
    else
    {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}
DX_DEVICE_TWIN_HANDLER_END

DX_DIRECT_METHOD_HANDLER(LightOnHandler, json, directMethodBinding, responseMsg)
{
    DX_GPIO_BINDING *led = (DX_GPIO_BINDING *)directMethodBinding->context;
    dx_gpioStateSet(led, true);
    return DX_METHOD_SUCCEEDED;
}
DX_DIRECT_METHOD_HANDLER_END

DX_DIRECT_METHOD_HANDLER(LightOffHandler, json, directMethodBinding, responseMsg)
{
    DX_GPIO_BINDING *led = (DX_GPIO_BINDING *)directMethodBinding->context;
    dx_gpioStateSet(led, false);
    return DX_METHOD_SUCCEEDED;
}
DX_DIRECT_METHOD_HANDLER_END

static void StartupReport(bool connected)
{
    // This is the first connect so update device start time UTC and software version
    dx_deviceTwinReportValue(&dt_deviceStartUtc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer)));
    snprintf(msgBuffer, sizeof(msgBuffer), "Sample version: %s, DevX version: %s", SAMPLE_VERSION_NUMBER, AZURE_SPHERE_DEVX_VERSION);
    dx_deviceTwinReportValue(&dt_softwareVersion, msgBuffer);
    dx_azureUnregisterConnectionChangedNotification(StartupReport);
}

static void NetworkConnectionState(bool connected)
{
    azure_connected = connected;
    dx_gpioStateSet(&gpio_network_led, connected);
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));

    dx_azureRegisterConnectionChangedNotification(StartupReport);
    dx_azureRegisterConnectionChangedNotification(NetworkConnectionState);
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

    if (!dx_configParseCmdLineArguments(argc, argv, &dx_config))
    {
        return dx_getTerminationExitCode();
    }

    InitPeripheralsAndHandlers();

    // This call blocks until termination requested
    dx_eventLoopRun();

    ClosePeripheralsAndHandlers();
    return dx_getTerminationExitCode();
}