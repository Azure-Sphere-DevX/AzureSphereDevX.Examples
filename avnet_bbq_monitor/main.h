
#include "hw/bbq_monitor.h" // Hardware definition
#include "app_exit_codes.h"
#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_direct_methods.h"
#include "dx_device_twins.h"
#include "dx_intercore.h"
#include "thermo_click_rt_app.h"
#include "dx_pwm.h"
#include "dx_version.h"
#include <applibs/log.h>
#include <applibs/applications.h>

// Use main.h to define all your application definitions, message properties/contentProperties,
// bindings and binding sets.

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "" // TODO insert your PnP model ID if your application supports PnP

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
typedef enum { STEAK = 0, POLO = 1, SWINE = 2 } meat_t;
typedef enum { RARE = 125, MEDIUM_RARE = 130, MEDIUM = 135, MEDIUM_WELL = 140, WELL_DONE = 155 } steak_order_t;

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/
// Device Twin Declarations
static void dt_target_meat_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void dt_target_temp_steak_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void dt_target_temp_polo_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void dt_target_temp_swine_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void dt_temp_over_done_handler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);

static void buzz_click_alarm(bool, DX_PWM_BINDING *);

// Timer Declarations
static void read_and_process_sensor_data_handler(EventLoopTimer *eventLoopTimer);

// Real-time handler Declarations
static void receive_msg_handler(void *data_block, ssize_t message_length);

/****************************************************************************************
 * Telemetry message buffer property sets
 ****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
// TODO: Remove comments to use the global message buffer for sending telemetry
//#define JSON_MESSAGE_BYTES 256
// static char msgBuffer[JSON_MESSAGE_BYTES] = {0};

// TODO: Define telemetry message properties here, for example . . .
// static DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "hvac"},
//                                                   &(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
//                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

// TODO: Remove comments to define contentProperties for sending telemetry
// static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

// Data structures to manage messages to the real-time app (HL to RT) and data from the
// real-time app (RT to HL)
IC_COMMAND_BLOCK_THERMO_CLICK_HL_TO_RT ic_tx_block;
IC_COMMAND_BLOCK_THERMO_CLICK_RT_TO_HL ic_rx_block;

/****************************************************************************************
 * Bindings
 ****************************************************************************************/
// Device Twin Bindings
static DX_DEVICE_TWIN_BINDING dt_target_meat = {.propertyName = "targetMeat", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_target_meat_handler};
static DX_DEVICE_TWIN_BINDING dt_target_temp_steak = {.propertyName = "ttSteak", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_target_temp_steak_handler};
static DX_DEVICE_TWIN_BINDING dt_target_temp_polo = {.propertyName = "ttPolo", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_target_temp_polo_handler};
static DX_DEVICE_TWIN_BINDING dt_target_temp_swine = {.propertyName = "ttSwine", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_target_temp_swine_handler};
static DX_DEVICE_TWIN_BINDING dt_target_over_temp = {.propertyName = "ttOverDone", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_temp_over_done_handler};

// Timer Bindings
static DX_TIMER_BINDING tmr_read_and_process_sensor_data = {
    .repeat = &(struct timespec){10, 0}, .name = "ReadAndProcessSensorData", .handler = read_and_process_sensor_data_handler};

// Inter Core Bindings
DX_INTERCORE_BINDING intercore_thermo_click_binding = {.sockFd = -1,
                                                       .nonblocking_io = true,
                                                       .rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
                                                       .interCoreCallback = receive_msg_handler,
                                                       .intercore_recv_block = &ic_rx_block,
                                                       .intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_THERMO_CLICK_RT_TO_HL)};

// PWM structures
static DX_PWM_CONTROLLER pwm_buzz_controller = {.controllerId = PWM_CLICK_CONTROLLER, .name = "PWM Click Controller"};
static DX_PWM_BINDING pwm_buzz_click = {.pwmController = &pwm_buzz_controller, .channelId = 1, .name = "click 2 buzz"};

/****************************************************************************************
 * Binding sets
 ****************************************************************************************/
// TODO: Update each binding set below with the bindings defined above.  Add bindings by reference, i.e., &dt_desired_sample_rate
// These sets are used by the initailization code.

DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_target_meat, &dt_target_temp_steak, &dt_target_temp_polo, &dt_target_temp_swine, &dt_target_over_temp};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_read_and_process_sensor_data};
static DX_PWM_BINDING *pwm_bindings[] = {&pwm_buzz_click};
