
#include "hw/sample_appliance.h" // Hardware definition
#include "app_exit_codes.h"
#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_direct_methods.h"
#include "dx_device_twins.h"
#include "dx_version.h"
#include <applibs/log.h>
#include <applibs/applications.h>

// Use main.h to define all your application definitions, message properties/contentProperties,
// bindings and binding sets.

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "" //TODO insert your PnP model ID if your application supports PnP

// Details on how to connect your application using an ethernet adaptor
// https://docs.microsoft.com/en-us/azure-sphere/network/connect-ethernet
#define NETWORK_INTERFACE "wlan0"

#define SAMPLE_VERSION_NUMBER "1.0"
#define ONE_MS 1000000

DX_USER_CONFIG dx_config;

/****************************************************************************************
 * Avnet IoTConnect Support
 ****************************************************************************************/
// TODO: If the application will connect to Avnet's IoTConnect platform enable the 
// #define below
//#define USE_AVNET_IOTCONNECT

/****************************************************************************************
 * Application defines
 ****************************************************************************************/
typedef enum {
    STEAK = 0,
    POLO  = 1,
    SWINE = 2
} meat_t;

typedef enum{
    RARE = 125,
    MEDIUM_RARE = 130,
    MEDIUM      = 135,
    MEDIUM_WELL = 140,
    WELL_DONE   = 155
} steak_order_t;

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/
static void dt_target_meat_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void dt_target_temp_steak_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void dt_target_temp_polo_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void dt_target_temp_swine_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void dt_temp_over_done_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);

/****************************************************************************************
 * Telemetry message buffer property sets
 ****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
// TODO: Remove comments to use the global message buffer for sending telemetry
//#define JSON_MESSAGE_BYTES 256
//static char msgBuffer[JSON_MESSAGE_BYTES] = {0};

// TODO: Define telemetry message properties here, for example . . . 
//static DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "hvac"}, 
//                                                   &(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
//                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

// TODO: Remove comments to define contentProperties for sending telemetry
//static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

/****************************************************************************************
 * Bindings
 ****************************************************************************************/
static DX_DEVICE_TWIN_BINDING dt_target_meat = {.propertyName = "targetMeat", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_target_meat_handler};
static DX_DEVICE_TWIN_BINDING dt_target_temp_steak = {.propertyName = "ttSteak", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_target_temp_steak_handler};
static DX_DEVICE_TWIN_BINDING dt_target_temp_polo = {.propertyName = "ttPolo", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_target_temp_polo_handler};
static DX_DEVICE_TWIN_BINDING dt_target_temp_swine = {.propertyName = "ttSwine", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_target_temp_swine_handler};
static DX_DEVICE_TWIN_BINDING dt_target_over_temp = {.propertyName = "ttOverDone", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_temp_over_done_handler};

/****************************************************************************************
 * Binding sets
 ****************************************************************************************/
// TODO: Update each binding set below with the bindings defined above.  Add bindings by reference, i.e., &dt_desired_sample_rate
// These sets are used by the initailization code.

DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_target_meat, &dt_target_temp_steak, &dt_target_temp_polo, &dt_target_temp_swine, &dt_target_over_temp};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {};
DX_TIMER_BINDING *timer_bindings[] = {};
