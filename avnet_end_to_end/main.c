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
    double temperature = 36.0;
    double humidity = 55.0;
    double pressure = 1100;
    static int msgId = 0;

    // Only send telemetry if we're connected to IoTConnect
    if (dx_isAvnetConnected()) {

        // Serialize telemetry as JSON
        bool serialization_result = dx_avnetJsonSerialize(msgBuffer, sizeof(msgBuffer), NULL, 4, 
            DX_JSON_INT, "MsgId", msgId++, 
            DX_JSON_DOUBLE, "Temperature", temperature, 
            DX_JSON_DOUBLE, "Humidity", humidity, 
            DX_JSON_DOUBLE, "Pressure", pressure);

        if (serialization_result) {

            Log_Debug("%s\n", msgBuffer);

            dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);

        } else {
            Log_Debug("JSON Serialization failed: Buffer too small\n");
            dx_terminate(APP_ExitCode_Telemetry_Buffer_Too_Small);
        }
    }
}
DX_TIMER_HANDLER_END

static DX_TIMER_HANDLER(report_properties_handler)
{
    float temperature = 25.05f;
    double humidity = 60.25;

    if (dx_isAzureConnected()) {

        // Update twin with current UTC (Universal Time Coordinate) in ISO format
        dx_deviceTwinReportValue(&dt_reported_utc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer)));

        // The type passed in must match the Divice Twin Type DX_DEVICE_TWIN_FLOAT
        dx_deviceTwinReportValue(&dt_reported_temperature, &temperature);

        // The type passed in must match the Divice Twin Type DX_DEVICE_TWIN_DOUBLE
        dx_deviceTwinReportValue(&dt_reported_humidity, &humidity);
    }
}
DX_TIMER_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_desired_sample_rate_handler, deviceTwinBinding)
{
    int sample_rate_seconds = *(int *)deviceTwinBinding->propertyValue;

    // validate data is sensible range before applying
    if (sample_rate_seconds >= 0 && sample_rate_seconds <= 120) {

        dx_timerChange(&tmr_publish_message, &(struct timespec){sample_rate_seconds, 0});

        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);

    } else {
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
    }

    /*	Casting device twin state examples

            float value = *(float*)deviceTwinBinding->propertyValue;
            double value = *(double*)deviceTwinBinding->propertyValue;
            int value = *(int*)deviceTwinBinding->propertyValue;
            bool value = *(bool*)deviceTwinBinding->propertyValue;
            char* value = (char*)deviceTwinBinding->propertyValue;
    */
}
DX_DEVICE_TWIN_HANDLER_END

// Direct method name = LightControl, json payload = {"State": true }
static DX_DIRECT_METHOD_HANDLER(LightControlHandler, json, directMethodBinding, responseMsg)
{
    char state_str[] = "State";
    bool requested_state;

    JSON_Object *jsonObject = json_value_get_object(json);
    if (jsonObject == NULL) {
        return DX_METHOD_FAILED;
    }

    requested_state = (bool)json_object_get_boolean(jsonObject, state_str);

    dx_gpioStateSet(&gpio_led, requested_state);

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
    if (connected) {
        dx_deviceTwinReportValue(&dt_deviceConnectUtc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer)));
    }

    dx_gpioStateSet(&gpio_network_led, connected);
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_avnetConnect(&dx_config, NETWORK_INTERFACE);
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