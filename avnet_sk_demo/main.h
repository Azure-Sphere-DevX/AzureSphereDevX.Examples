#pragma once

#include "hw/sample_appliance.h" // Hardware definition
#include "build_options.h"

// DevX libraries
#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_exit_codes.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_version.h"
#ifdef M4_INTERCORE_COMMS
#include "dx_intercore.h"
#endif // M4_INTERCORE_COMMS
#ifdef USE_IOT_CONNECT
#include "dx_avnet_iot_connect.h"
#endif // USE_IOT_CONNECT

// Azure Sphere SDK libraries
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/wificonfig.h>
#include <applibs/powermanagement.h>

// Local header files
#include "app_exit_codes.h"
#include "i2c.h"
#ifdef OLED_SD1306
#include "oled.h"
#endif // OLED_SD1306
#ifdef M4_INTERCORE_COMMS
#include "als_pt19_light_sensor.h"
#endif // M4_INTERCORE_COMMS

#define NETWORK_INTERFACE "wlan0"
#define SAMPLE_VERSION_NUMBER "1.0"
#define ONE_MS 1000000

// Forward declarations
//static DX_DIRECT_METHOD_RESPONSE_CODE LightControlHandler(JSON_Value *json, DX_DIRECT_METHOD_BINDING *directMethodBinding, char **responseMsg);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_desired_sample_rate_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_gpio_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_oled_message_handler);
static DX_DECLARE_DIRECT_METHOD_HANDLER(dm_halt_device_handler);
static DX_DECLARE_DIRECT_METHOD_HANDLER(dm_restart_device_handler);
static DX_DECLARE_DIRECT_METHOD_HANDLER(dm_set_sensor_poll_period);
static DX_DECLARE_TIMER_HANDLER(delay_restart_timer_handler);
static DX_DECLARE_TIMER_HANDLER(monitor_wifi_network_handler);
static DX_DECLARE_TIMER_HANDLER(read_sensors_handler);
static void publish_message_handler(void);
#ifdef OLED_SD1306
static DX_DECLARE_TIMER_HANDLER(UpdateOledEventHandler);
#endif // OLED_SD1306
static void ReadWifiConfig(bool outputDebug);
static DX_DECLARE_TIMER_HANDLER(ButtonPressCheckHandler);
#ifdef IOT_HUB_APPLICATION
static void SendButtonTelemetry(const char* telemetry_key, GPIO_Value_Type button_state);
#endif // IOT_HUB_APPLICATION
static void ProcessButtonState(GPIO_Value_Type new_state, GPIO_Value_Type* old_state, const char* telemetry_key);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_debug_handler);
#ifdef M4_INTERCORE_COMMS
static void alsPt19_receive_msg_handler(void *data_block, ssize_t message_length);
#endif // M4_INTERCORE_COMMS

DX_USER_CONFIG dx_config;

/****************************************************************************************
 * Telemetry message buffer and property sets
*****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
#define JSON_MESSAGE_BYTES 512
#ifdef IOT_HUB_APPLICATION
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};
#endif // IOT_HUB_APPLICATION        

#ifdef IOT_HUB_APPLICATION
static DX_MESSAGE_PROPERTY *messageProperties[] =   {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "SK-Demo"}, 
                                                    &(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
                                                    &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};
#endif //IOT_HUB_APPLICATION

/****************************************************************************************
 * Global Variables
 ****************************************************************************************/
#ifdef USE_WEB_PROXY
DX_PROXY_PROPERTIES proxy = {.proxyAddress = PROXY_ADDRESS,
                             .proxyPort = PROXY_PORT,
                             .proxyUsername = PROXY_USERNAME,
                             .proxyPassword = PROXY_PASSWORD,
                             .noProxyAdresses = NO_PROXY_ADDRESSES
};
#endif // USE_WEB_PROXY



// Array with messages from Azure
//                                123456789012345678901
char oled_ms1[CLOUD_MSG_SIZE] = {"        Avnet         "};
char oled_ms2[CLOUD_MSG_SIZE] = {"     Azure Sphere     "};
char oled_ms3[CLOUD_MSG_SIZE] = {"     Starter Kit      "};
char oled_ms4[CLOUD_MSG_SIZE] = {"                      "};

bool sensor_debug_enabled = true;
extern bool RTCore_connected;

// Global variables to hold sensor readings. 
AccelerationgForce acceleration_g;
AngularRateDegreesPerSecond angular_rate_dps;
float lsm6dso_temperature;
float pressure_hPa;
float lps22hh_temperature;
float altitude;
double light_sensor;
network_var network_data;

/****************************************************************************************
 * GPIO Peripherals
 ****************************************************************************************/
static DX_GPIO_BINDING buttonA =      {.pin = SAMPLE_BUTTON_1,     .name = "buttonA",      .direction = DX_INPUT,   .detect = DX_GPIO_DETECT_LOW};
static DX_GPIO_BINDING buttonB =      {.pin = SAMPLE_BUTTON_2,     .name = "buttonB",      .direction = DX_INPUT,   .detect = DX_GPIO_DETECT_LOW};
static DX_GPIO_BINDING userLedRed =   {.pin = SAMPLE_RGBLED_RED,   .name = "userLedRed",   .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING userLedGreen = {.pin = SAMPLE_RGBLED_GREEN, .name = "userLedGreen", .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING userLedBlue =  {.pin = SAMPLE_RGBLED_BLUE,  .name = "userLedBlue",  .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING wifiLed =      {.pin = SAMPLE_WIFI_LED,     .name = "WifiLed",      .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING appLed =       {.pin = SAMPLE_APP_LED,      .name = "appLed",       .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING clickRelay1 =  {.pin = RELAY_CLICK2_RELAY1, .name = "relay1",       .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = false};
static DX_GPIO_BINDING clickRelay2 =  {.pin = RELAY_CLICK2_RELAY2, .name = "relay2",       .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = false};

/****************************************************************************************
 * Device Twins
 ****************************************************************************************/
// Read/Write Device Twin Bindings
static DX_DEVICE_TWIN_BINDING dt_user_led_red =        {.propertyName = "userLedRed",       .twinType = DX_DEVICE_TWIN_BOOL,  .handler = dt_gpio_handler, .context = &userLedRed};	
static DX_DEVICE_TWIN_BINDING dt_user_led_green =      {.propertyName = "userLedGreen",     .twinType = DX_DEVICE_TWIN_BOOL,  .handler = dt_gpio_handler, .context = &userLedGreen};
static DX_DEVICE_TWIN_BINDING dt_user_led_blue =       {.propertyName = "userLedBlue",      .twinType = DX_DEVICE_TWIN_BOOL,  .handler = dt_gpio_handler, .context = &userLedBlue};
static DX_DEVICE_TWIN_BINDING dt_wifi_led =            {.propertyName = "wifiLed",          .twinType = DX_DEVICE_TWIN_BOOL,  .handler = dt_gpio_handler, .context = &wifiLed};
static DX_DEVICE_TWIN_BINDING dt_app_led =             {.propertyName = "appLed",           .twinType = DX_DEVICE_TWIN_BOOL,  .handler = dt_gpio_handler, .context = &appLed};
static DX_DEVICE_TWIN_BINDING dt_relay1 =              {.propertyName = "clickBoardRelay1", .twinType = DX_DEVICE_TWIN_BOOL,  .handler = dt_gpio_handler, .context = &clickRelay1};
static DX_DEVICE_TWIN_BINDING dt_relay2 =              {.propertyName = "clickBoardRelay2", .twinType = DX_DEVICE_TWIN_BOOL,  .handler = dt_gpio_handler, .context = &clickRelay2};
static DX_DEVICE_TWIN_BINDING dt_desired_sample_rate = {.propertyName = "sensorPollPeriod", .twinType = DX_DEVICE_TWIN_INT,   .handler = dt_desired_sample_rate_handler};
static DX_DEVICE_TWIN_BINDING dt_oled_line1 =          {.propertyName = "OledDisplayMsg1", .twinType = DX_DEVICE_TWIN_STRING, .handler = dt_oled_message_handler, .context = oled_ms1 };
static DX_DEVICE_TWIN_BINDING dt_oled_line2 =          {.propertyName = "OledDisplayMsg2", .twinType = DX_DEVICE_TWIN_STRING, .handler = dt_oled_message_handler, .context = oled_ms2 };
static DX_DEVICE_TWIN_BINDING dt_oled_line3 =          {.propertyName = "OledDisplayMsg3", .twinType = DX_DEVICE_TWIN_STRING, .handler = dt_oled_message_handler, .context = oled_ms3 };
static DX_DEVICE_TWIN_BINDING dt_oled_line4 =          {.propertyName = "OledDisplayMsg4", .twinType = DX_DEVICE_TWIN_STRING, .handler = dt_oled_message_handler, .context = oled_ms4 };
static DX_DEVICE_TWIN_BINDING dt_enable_debug =        {.propertyName = "enableDebug",     .twinType = DX_DEVICE_TWIN_BOOL,   .handler = dt_debug_handler};	

// Read only Device Twin Bindings
static DX_DEVICE_TWIN_BINDING dt_version_string = {.propertyName = "versionString", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_manufacturer =   {.propertyName = "manufacturer", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_model =          {.propertyName = "model", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_ssid =           {.propertyName = "ssid", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_freq =           {.propertyName = "freq", .twinType = DX_DEVICE_TWIN_INT};
static DX_DEVICE_TWIN_BINDING dt_bssid =          {.propertyName = "bssid", .twinType = DX_DEVICE_TWIN_STRING};

/****************************************************************************************
 * Direct Methods
 ****************************************************************************************/
static DX_DIRECT_METHOD_BINDING dm_sensor_poll_time = {.methodName = "setSensorPollTime", .handler = dm_set_sensor_poll_period}; // {"pollTime": <integer>}
static DX_DIRECT_METHOD_BINDING dm_reboot_control =   {.methodName = "rebootDevice", .handler = dm_restart_device_handler};   // {"delayTime": <integer>}
static DX_DIRECT_METHOD_BINDING dm_halt_control =     {.methodName = "haltApplication", .handler = dm_halt_device_handler};    // {}

/****************************************************************************************
 * Timers
 ****************************************************************************************/
static DX_TIMER_BINDING tmr_monitor_wifi_network = {.period = {30, 0}, .name = "tmr_monitor_wifi_network", .handler = monitor_wifi_network_handler};
static DX_TIMER_BINDING tmr_read_sensors = {.period = {SENSOR_READ_PERIOD_SECONDS, 0}, .name = "tmr_read_sensors", .handler = read_sensors_handler};
static DX_TIMER_BINDING tmr_reboot = {.period = {0, 0}, .name = "tmr_reboot", .handler = delay_restart_timer_handler};
static DX_TIMER_BINDING buttonPressCheckTimer = {.period = {0, ONE_MS*10}, .name = "buttonPressCheckTimer", .handler = ButtonPressCheckHandler};
#ifdef OLED_SD1306
static DX_TIMER_BINDING oled_timer = {.period = {0, 100 * ONE_MS}, .name = "oledTimer", .handler = UpdateOledEventHandler};
#endif 

#ifdef M4_INTERCORE_COMMS
/****************************************************************************************
 * Inter Core Bindings
*****************************************************************************************/
IC_COMMAND_BLOCK_ALS_PT19 ic_control_block_alsPt19_light_sensor = {.cmd = IC_READ_SENSOR,
                                                                   .lightSensorLuxData = 0.0,
                                                                   .sensorData = 0,
                                                                   .sensorSampleRate = 0};

DX_INTERCORE_BINDING intercore_alsPt19_light_sensor = {
    .sockFd = -1,
    .nonblocking_io = true,
    .rtAppComponentId = "b2cec904-1c60-411b-8f62-5ffe9684b8ce",
    .interCoreCallback = alsPt19_receive_msg_handler,
    .intercore_recv_block = &ic_control_block_alsPt19_light_sensor,
    .intercore_recv_block_length = sizeof(ic_control_block_alsPt19_light_sensor)};

#endif // M4_INTERCORE_COMMS

// All bindings referenced in the folowing binding sets are initialised in the InitPeripheralsAndHandlers function
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_user_led_red, &dt_user_led_green, &dt_user_led_blue, &dt_wifi_led, 
                                                  &dt_app_led, &dt_relay1, &dt_relay2, &dt_desired_sample_rate, &dt_oled_line1, 
                                                  &dt_oled_line2, &dt_oled_line3, &dt_oled_line4, &dt_version_string, 
                                                  &dt_manufacturer, &dt_model, &dt_ssid, &dt_freq, &dt_bssid,
                                                  &dt_enable_debug};

DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {&dm_reboot_control, &dm_sensor_poll_time, &dm_halt_control};
DX_GPIO_BINDING *gpio_bindings[] = {&buttonA, &buttonB, &userLedRed, &userLedGreen, &userLedBlue, &wifiLed, &appLed, &clickRelay1, &clickRelay2};
#ifdef OLED_SD1306
DX_TIMER_BINDING *timer_bindings[] = {&tmr_monitor_wifi_network, &tmr_read_sensors, &tmr_reboot, &buttonPressCheckTimer, &oled_timer};
#else
DX_TIMER_BINDING *timer_bindings[] = {&tmr_monitor_wifi_network, &tmr_read_sensors, &tmr_reboot, &buttonPressCheckTimer};
#endif 
