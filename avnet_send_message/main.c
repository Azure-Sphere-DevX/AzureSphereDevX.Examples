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

static void publish_message_handler(EventLoopTimer *eventLoopTimer)
{

    static int pollTime = 15;

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    // Get any telemetry frequency updates from the IoTConnecty library
    int ioTConnectPollTime = dx_avnetGetTelemetryPeriod();

    // If the poll frequency is 0, then IoT Connect requested that the 
    // application stop sending telemetry, return without sending any
    // telemetry.
    if(ioTConnectPollTime == 0){
        return;
    }

    // Verify we have received an updated frequency time from IoTConnect
    if(ioTConnectPollTime > 0){
        if (ioTConnectPollTime != pollTime){
            pollTime = ioTConnectPollTime;
            dx_timerChange(&publish_message, &(struct timespec){.tv_sec = pollTime, .tv_nsec = 0});
        }
    }

    double temperature = 36.0;
    double humidity = 55.0;
    double pressure = 1100;
    static int msgId = 0;

    if (dx_isAvnetConnected()) {

        // Serialize telemetry as JSON
        bool serialization_result =
            dx_avnetJsonSerialize(msgBuffer, sizeof(msgBuffer), 4, DX_JSON_INT, "MsgId", msgId++, 
                DX_JSON_DOUBLE, "Temperature", temperature, 
                DX_JSON_DOUBLE, "Humidity", humidity, 
                DX_JSON_DOUBLE, "Pressure", pressure);

        if (serialization_result) {

            Log_Debug("%s\n", msgBuffer);

            // Neerav, put the call to your new validation routine here and remove the dx_azurePublish() call since the
            // new routine will send the telemetry with the correct mt value.

            dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);

        } else {
            Log_Debug("JSON Serialization failed: Buffer too small\n");
            dx_terminate(APP_ExitCode_Telemetry_Buffer_Too_Small);
        }

        if (dt_desired_temperature.propertyUpdated) {
            if (desired_temperature > temperature) {
                Log_Debug("It's too hot\n");
            } else if (desired_temperature < temperature) {
                Log_Debug("The temperature too cold\n");
            } else if (desired_temperature == temperature) {
                Log_Debug("The temperature is just right\n");
            }

            // now update the reported temperature device twin
            dx_deviceTwinReportValue(&dt_reported_temperature, &temperature);
        }
    }
}

static void dt_desired_temperature_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    // validate data is sensible range before applying
    // For example set HVAC system desired temperature in celsius
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_DOUBLE && *(double *)deviceTwinBinding->propertyValue >= 18.0 &&
        *(int *)deviceTwinBinding->propertyValue <= 30.0) {

        desired_temperature = *(double *)deviceTwinBinding->propertyValue;

        // If IoT Connect Pattern is to respond with the Device Twin Key Value then do the following
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
    }
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timers.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_avnetConnect(&dx_config, NETWORK_INTERFACE);
    dx_timerSetStart(timers, NELEMS(timers));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timers, NELEMS(timers));
    dx_deviceTwinUnsubscribe();
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