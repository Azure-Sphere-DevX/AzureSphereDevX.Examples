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


// Create children dynamically
//
// You may want to create children devices dynamically if you have a gateway device and you start
// to bring on children sensors.  As the sensors connect to the gateway, the application can dynamically
// create/associate children devices on the IoTConnect Gateway device.  
//
// The three peices of information needed are . . .
// 1. Unique device ID:  This could be a mac address or serial number of the child device/sensor
// 2. Tag:               The tag is a common string that the application can use when it associates a new child
//                       device with a gateway device in IoTConnect.  In IoTconnect you create a gateway template
//                       where you can define attributes (telemetry) and device twins associated with your child 
//                       device.  So the developer must know how to map a new device to a tag.
// 3. Device Name:       This is a user define device name that will be displayed in IoTConnect
//
// When a device is added to the gateway, the devX library also creates a gw_child_list_node_t linked list node with 
// the id and tag(tg).  The high level application can pass the gw_child_list_node_t pointer when sending telemetry
// and the library will properly construct the telemetry message for the child device.  The gw_child_list_node_t 
// node can be obtained by passing the childs id into dx_avnetFindChild(nonst char* id).
//
// typedef struct node {
//	struct node* next;
//	struct node* prev;
//  char tg[DX_AVNET_IOT_CONNECT_GW_FIELD_LEN];
//	char id[DX_AVNET_IOT_CONNECT_GW_FIELD_LEN];
// } gw_child_list_node_t;
//
// Once child devices have been created on the IoTConnect platform, the children id and tags will be sent to the application on startup.
// This way the the application only need to dynamically create children when new child devices are connected to the gateway.
static DX_TIMER_HANDLER(add_gw_children_handler)
{
    if (dx_isAvnetConnected()) {

        // Create child devices dynamically on the GW device
        dx_avnetCreateChildOnIoTConnect("temperatureChild01", "temperaturechilddevice", "tempSensorDrinkCooler");
        dx_avnetCreateChildOnIoTConnect("temperatureChild02", "temperaturechilddevice", "tempSensorFreezer");

        dx_avnetCreateChildOnIoTConnect("humidityChild01", "humiditychilddevice", "humiditySensorDrinkCooler");
        dx_avnetCreateChildOnIoTConnect("humidityChild02", "humiditychilddevice", "humiditySensorFreezer");

        dx_avnetCreateChildOnIoTConnect("pressureChild01", "pressurechilddevice", "pressureCityWater");
        dx_avnetCreateChildOnIoTConnect("pressureChild02", "pressurechilddevice", "pressureFilteredWater");

        // Stop the timer so this routine does not execute again
        dx_timerStop(&tmr_add_gw_children);
    }
}
DX_TIMER_HANDLER_END

static DX_TIMER_HANDLER(publish_message_handler)
{
    if (dx_isAvnetConnected()){
    
        sendChildDeviceTelemetry("temperatureChild01", "temperature", ((float)rand()/(float)(RAND_MAX)) * 5);
        sendChildDeviceTelemetry("temperatureChild02", "temperature", ((float)rand()/(float)(RAND_MAX)) * 2);

        sendChildDeviceTelemetry("humidityChild01", "humidity", ((float)rand()/(float)(RAND_MAX)) * 100);
        sendChildDeviceTelemetry("humidityChild02", "humidity", ((float)rand()/(float)(RAND_MAX)) * 100);

        sendChildDeviceTelemetry("pressureChild01", "pressure", ((float)rand()/(float)(RAND_MAX)) * 60);
        sendChildDeviceTelemetry("pressureChild02", "pressure", ((float)rand()/(float)(RAND_MAX)) * 50);

        Log_Debug("\n");

    }
}
DX_TIMER_HANDLER_END

static DX_TIMER_HANDLER(delete_gw_children_handler)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    if (dx_isAvnetConnected()) {

        // Get a pointer to the first child in the list
        gw_child_list_node_t* childNodePtr = dx_avnetGetFirstChild();

        // As long as we have a valid child pointer delete the child
        while(childNodePtr != NULL){

            // Delete the child device on IoTConnect, this call also
            // removes the child node from the list
            dx_avnetDeleteChildOnIoTConnect(childNodePtr->id);

            // Get the next child node
            childNodePtr = dx_avnetGetNextChild(childNodePtr);

        }

        // Stop the timers to stop the demo
        dx_timerStop(&tmr_publish_message);
        dx_timerStop(&tmr_delete_gw_children);
        Log_Debug("Wait for children deleted confirmations before exiting . . . !\n");
        
    }
}
DX_TIMER_HANDLER_END


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


void sendChildDeviceTelemetry(const char* id, const char* key, float value){


    // Create serialized telemetry as JSON
    bool serialization_result = 
        dx_avnetJsonSerialize(msgBuffer, sizeof(msgBuffer), dx_avnetFindChild(id),  1,
                            DX_JSON_DOUBLE, key, value);

    if (serialization_result) {

        Log_Debug("%s\n", msgBuffer);
        dx_azurePublish(msgBuffer, strlen(msgBuffer), NULL, 0, NULL);

    } else {
        Log_Debug("JSON Serialization failed: Buffer too small\n");
        dx_terminate(APP_ExitCode_Telemetry_Buffer_Too_Small);
    }
}