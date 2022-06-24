
#include "hw/sample_appliance.h" // Hardware definition
#include "app_exit_codes.h"
#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_direct_methods.h"
#include "dx_version.h"
#include "dx_config.h"
#include "dx_gpio.h"
#include "dx_pwm.h"
#include <applibs/log.h>
#include <applibs/applications.h>
#include "dx_intercore.h"
#include "pht_click.h"
#include "dx_uart.h"

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "" 

// Details on how to connect your application using an ethernet adaptor
// https://docs.microsoft.com/en-us/azure-sphere/network/connect-ethernet
#define NETWORK_INTERFACE "wlan0"

#define SAMPLE_VERSION_NUMBER "1.0"


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
#define ONE_MS 1000000
#define ONE_HUNDRED_MS 100000000

#define DEFAULT_SENSOR_POLL_PERIOD_SECONDS 0
#define DEFAULT_SENSOR_POLL_PERIOD_MS (ONE_MS * 100)

#define DEFAULT_SEND_TELEMETRY_PERIOD_SECONDS 5

#define MIN_POLL_TIME_MS 20
#define MAX_POLL_TIME_MS 60*1000 // 1 Minute

#define MIN_TELEMETRY_TX_PERIOD 1
#define MAX_TELEMETRY_TX_PERIOD (60*60) // 1 Hour

#define HISTORICAL_DATA_ARRAY_SIZE 32

#define OVER_UNDER_RANGE 2 // Degrees C 

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/
static void receive_msg_handler(void *data_block, ssize_t message_length);
static DX_DECLARE_TIMER_HANDLER(ReadSensorHandler);
static DX_DECLARE_TIMER_HANDLER(SendTelemetryHandler);
static DX_DECLARE_TIMER_HANDLER(ButtonPressCheckHandler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_set_sensor_polling_period_ms);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_set_telemetemetry_period_seconds);
static void updateHistoricalTempData(float tempData);

IC_COMMAND_BLOCK_PHT_CLICK_HL_TO_RT ic_tx_block;
IC_COMMAND_BLOCK_PHT_CLICK_RT_TO_HL ic_rx_block;

/****************************************************************************************
 * Telemetry message buffer property sets
 ****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
#define JSON_MESSAGE_BYTES 64
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};

static DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "TMF8801"}, 
                                                   &(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

/****************************************************************************************
 * Bindings
 ****************************************************************************************/
DX_INTERCORE_BINDING intercore_tempHum13_click_binding = {
    .sockFd = -1,
    .nonblocking_io = true,
    .rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
    .interCoreCallback = receive_msg_handler,
    .intercore_recv_block = &ic_rx_block,
    .intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_PHT_CLICK_RT_TO_HL)}; 

static DX_TIMER_BINDING readSensorTimer = {
    .repeat = &(struct timespec){DEFAULT_SENSOR_POLL_PERIOD_SECONDS, DEFAULT_SENSOR_POLL_PERIOD_MS}, 
    .name = "readSensorTimer", .handler = ReadSensorHandler};

static DX_TIMER_BINDING sendTelemetryTimer = {
    .repeat = &(struct timespec){DEFAULT_SEND_TELEMETRY_PERIOD_SECONDS, 0}, 
    .name = "sendTelemetryTimer", .handler = SendTelemetryHandler};

static DX_PWM_CONTROLLER pwm_led_controller = {.controllerId = SAMPLE_LED_PWM_CONTROLLER,
                                               .name = "PWM Click Controller"};

static DX_PWM_BINDING pwm_red_led = {
    .pwmController = &pwm_led_controller, .channelId = 0, .name = "red_led"};

static DX_PWM_BINDING pwm_green_led = {
    .pwmController = &pwm_led_controller, .channelId = 1, .name = "green_led"};

static DX_PWM_BINDING pwm_blue_led = {
    .pwmController = &pwm_led_controller, .channelId = 2, .name = "blue led"};                                         

static DX_DEVICE_TWIN_BINDING dt_desired_sample_rate_ms = {.propertyName = "sensorPollPeriod_ms",
                                                           .twinType = DX_DEVICE_TWIN_INT,
                                                           .handler = dt_set_sensor_polling_period_ms};

static DX_DEVICE_TWIN_BINDING dt_telemetry_tx_period_s = {.propertyName = "setTelemetrySendPeriod_seconds",
                                                           .twinType = DX_DEVICE_TWIN_INT,
                                                           .handler = dt_set_telemetemetry_period_seconds};

static DX_TIMER_BINDING buttonPressCheckTimer = {
    .period = {0, 1000000}, .name = "buttonPressCheckTimer", .handler = ButtonPressCheckHandler};                                                           

static DX_GPIO_BINDING buttonA = {
    .pin = SAMPLE_BUTTON_1, .name = "buttonA", .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW};

static DX_GPIO_BINDING buttonB = {
    .pin = SAMPLE_BUTTON_2, .name = "buttonB", .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW};                                            
                                                          
/****************************************************************************************
 * Binding sets
 ****************************************************************************************/
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_desired_sample_rate_ms,
                                                  &dt_telemetry_tx_period_s};
static DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
static DX_GPIO_BINDING *gpio_bindings[] = {&buttonA, &buttonB};
static DX_TIMER_BINDING *timer_bindings[] = {&readSensorTimer, &sendTelemetryTimer, &buttonPressCheckTimer};
static DX_PWM_BINDING *pwm_bindings[] = {&pwm_red_led, &pwm_green_led, &pwm_blue_led};
