#pragma once

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
#include <applibs/log.h>
#include <applibs/applications.h>
#include "dx_avnet_iot_connect.h"
#include "netBooter.h"

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
#define USE_AVNET_IOTCONNECT

// eth0 Ethernet settings.  Note that this  is a direct  network
// connection between the NetBoot device and the Guardian100
#define MY_IP       "10.0.0.1"

// Note if you change the Device IP address, you must also make the same
// change in the app_manifest.json file
#define DEVICE_IP   "10.0.0.2"

/****************************************************************************************
 * Application defines
 ****************************************************************************************/
// Enumeration to pass network status to setConectionStatusLed()
typedef enum {
	RGB_Invalid  = 0,
    RGB_No_Network = 1,        // No network connection
    RGB_Network_Connected = 2, // Connected to Azure, not authenticated
    RGB_IoT_Hub_Connected = 3, // Connected and authenticated to IoT Hub

} RGB_Status;

static const int networkReadytimerPollPeriodSeconds = 1;
static const int networkReadytimerPollPeriodNanoSeconds = 0 * 1000;

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/
static void setConnectionStatusLed(RGB_Status);
static void ProcessButtonState(GPIO_Value_Type, GPIO_Value_Type* , const char* );

static DX_DECLARE_TIMER_HANDLER(update_network_led_handler);
static DX_DECLARE_TIMER_HANDLER(NetworkReadyPollTimerEventHandler);
static DX_DECLARE_TIMER_HANDLER(powerMonitorReadData);
static DX_DECLARE_TIMER_HANDLER(ButtonPressCheckHandler);

// Device Twin Handlers
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_dev1_enable_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_dev2_enable_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_telemetry_period_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_gpio_handler);

bool dev1Enabled = true;
bool dev2Enabled = true;
float dev1Current = 0.0F;
float dev2Current = 0.0F;
extern bool relay_1_enabled;
extern bool relay_2_enabled;

/****************************************************************************************
 * Bindings
 ****************************************************************************************/
// Timer Bindings
static DX_TIMER_BINDING tmr_update_network_led = {.period = {2, 0}, 
                                                  .name = "tmr_update_network_led", 
                                                  .handler = update_network_led_handler};
static DX_TIMER_BINDING tmr_networkReady =       {.period = {networkReadytimerPollPeriodSeconds, networkReadytimerPollPeriodNanoSeconds}, 
                                                  .name = "tmr_check_network", 
                                                  .handler = NetworkReadyPollTimerEventHandler};
static DX_TIMER_BINDING tmr_readPwrMonitor =     {.period = {15, 0}, 
                                                  .name = "tmr_read_power_monitor", 
                                                  .handler = powerMonitorReadData};
static DX_TIMER_BINDING tmr_buttonPress =        {.period = {0, ONE_MS*10}, 
                                                  .name = "buttonPressCheckTimer", 
                                                  .handler = ButtonPressCheckHandler};

// GPIO Bindings

static DX_GPIO_BINDING red_led =    {.pin = SAMPLE_RGBLED_RED, 
                                     .name = "Red LED", 
                                     .direction = DX_OUTPUT, 
                                     .initialState = GPIO_Value_Low, 
                                     .invertPin = true};                  
static DX_GPIO_BINDING green_led =   {.pin = SAMPLE_RGBLED_GREEN, 
                                      .name = "Green LED", 
                                      .direction = DX_OUTPUT, 
                                      .initialState = GPIO_Value_Low, 
                                      .invertPin = true};
static DX_GPIO_BINDING blue_led =    {.pin = SAMPLE_RGBLED_BLUE, 
                                      .name = "Blue LED", 
                                      .direction = DX_OUTPUT, 
                                      .initialState = GPIO_Value_Low, 
                                      .invertPin = true};
static DX_GPIO_BINDING buttonA =     {.pin = SAMPLE_BUTTON_1,     
                                      .name = "buttonA",      
                                      .direction = DX_INPUT,   
                                      .detect = DX_GPIO_DETECT_LOW};
static DX_GPIO_BINDING buttonB =     {.pin = SAMPLE_BUTTON_2,     
                                      .name = "buttonB",      
                                      .direction = DX_INPUT,   
                                      .detect = DX_GPIO_DETECT_LOW};
static DX_GPIO_BINDING clickRelay1 =  {.pin = RELAY_CLICK2_RELAY1, 
                                       .name = "relay1",       
                                       .direction = DX_OUTPUT,  
                                       .initialState = GPIO_Value_High, 
                                       .invertPin = false};
static DX_GPIO_BINDING clickRelay2 =  {.pin = RELAY_CLICK2_RELAY2, 
                                       .name = "relay2",       
                                       .direction = DX_OUTPUT,  
                                       .initialState = GPIO_Value_High, 
                                       .invertPin = false};

// Device Twin Bindings

static DX_DEVICE_TWIN_BINDING dt_dev1_enabled =     {.propertyName = "port1Enabled", 
                                                     .twinType = DX_DEVICE_TWIN_BOOL, 
                                                     .handler = dt_dev1_enable_handler};

static DX_DEVICE_TWIN_BINDING dt_dev2_enabled =     {.propertyName = "port2Enabled", 
                                                     .twinType = DX_DEVICE_TWIN_BOOL, 
                                                     .handler = dt_dev2_enable_handler}; 

static DX_DEVICE_TWIN_BINDING dt_telemetry_period = {.propertyName = "telemetryPeriodSeconds", 
                                                     .twinType = DX_DEVICE_TWIN_INT, 
                                                     .handler = dt_telemetry_period_handler}; 

static DX_DEVICE_TWIN_BINDING dt_relay1 =           {.propertyName = "clickBoardRelay1", 
                                                     .twinType = DX_DEVICE_TWIN_BOOL,  
                                                     .handler = dt_gpio_handler, 
                                                     .context = &clickRelay1};
static DX_DEVICE_TWIN_BINDING dt_relay2 =           {.propertyName = "clickBoardRelay2", 
                                                     .twinType = DX_DEVICE_TWIN_BOOL,  
                                                     .handler = dt_gpio_handler, 
                                                     .context = &clickRelay2};

/****************************************************************************************
 * Binding sets
 ****************************************************************************************/
// TODO: Update each binding set below with the bindings defined above.  Add bindings by reference, i.e., &dt_desired_sample_rate
// These sets are used by the initailization code.

DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_dev1_enabled, &dt_dev2_enabled, &dt_telemetry_period, &dt_relay1, &dt_relay2};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {&red_led, &green_led, &blue_led, &buttonA, &buttonB, &clickRelay1, &clickRelay2};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_update_network_led, &tmr_networkReady, &tmr_readPwrMonitor, &tmr_buttonPress};
