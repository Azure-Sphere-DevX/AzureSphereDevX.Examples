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
#include "dx_device_twins.h"
#include "dx_version.h"
#include "dx_uart.h"
#include "dx_intercore.h"
#include "pwr_meter_rt_app.h"
#include <applibs/log.h>
#include <applibs/applications.h>

// Details on how to connect your application using an ethernet adaptor
// https://docs.microsoft.com/en-us/azure-sphere/network/connect-ethernet
#define NETWORK_INTERFACE "wlan0"

#define SAMPLE_VERSION_NUMBER "1.0"
#define ONE_MS 1000000

DX_USER_CONFIG dx_config;

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

#define NO_PRODUCT_FOUND -1

typedef struct min_max {
	float Mininum;
	float Maximum;
} min_max_t;

#define ON 1
#define OFF 0

enum  product_enum_t {
	ESPRESSO = 0,
	PROTEIN_SHAKE = 1,
	MILK_SHAKE = 2,
	NO_PRODUCT = -1,
};

enum  product_size_enum_t {
	SMALL = 1,
	MEDIUM = 2,
	LARGE = 3,
	XLARGE = 4,
	NO_SIZE = -1,
};

typedef struct {
	double minRunTime;
	double maxRunTime;
	char* productName;
	int productEnum;
	char* productSize;
	int productSizeEnum;

} product_t;

/****************************************************************************************
 * Global Variables
 ****************************************************************************************/

static product_t productArray[] = { {.minRunTime = 15.000,.maxRunTime = 55.000,.productName = "Espresso",.productEnum = ESPRESSO,.productSize = "Small",.productSizeEnum = SMALL},
									{.minRunTime = 55.001,.maxRunTime = 75.000,.productName = "Espresso",.productEnum = ESPRESSO,.productSize = "Medium",.productSizeEnum = MEDIUM},
									{.minRunTime = 75.001,.maxRunTime = 120.000,.productName = "Espresso",.productEnum = ESPRESSO,.productSize = "Large",.productSizeEnum = LARGE}};

// on/off variables
static bool bDeviceIsOn = false;
static bool bLastDeviceIsOn = false;

// brewing variables
static bool bBrewing = false;
static bool bLastBrewing = false;

// Telemetry timeout variables
static struct timeval  lastTelemetryTime;
static struct timeval  newTelemetryTime;

// Runtime variables
static struct timeval devStartTime;
static struct timeval devStopTime;
static struct timeval devRunTime;
static double deviceRunTime = 0.0F;

// Power variables
static float fVoltage = NAN;

static float fCurrent = NAN;

static float fMaxTimeBetweenD2CMessages = 10; //Seconds.  This will be overwritten by the device twin update

static float fMinCurrentThreshold = 0.0F; // The minimum voltage to declare a device on.  This can be updated by the device twin

const float ON_OFF_LEAKAGE = 0.009F; // We have seen the MCP39F511 report very small currents even with nothing plugged in.  
									 // This fudge factor keeps us from having false on/off detections. 

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/

// Device Twin Handlers
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_generic_float_handler);

// Timer Handlers
static DX_DECLARE_TIMER_HANDLER(update_network_led_handler);
static DX_DECLARE_TIMER_HANDLER(tmr_get_current_data_handler);

static void setConnectionStatusLed(RGB_Status newNetworkStatus);
static void checkAndSendTelemetry(int productIndex);

// RT App Handler
static void receive_msg_handler(void *data_block, ssize_t message_length);

IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT ic_tx_block_sample;
IC_COMMAND_BLOCK_PWR_METER_RT_TO_HL ic_rx_block_sample;

/****************************************************************************************
 * Telemetry message buffer property sets
 ****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub
#define JSON_MESSAGE_BYTES 256
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};

// Define telemetry message formats
static const char cstrDeviceTelemetryJson[] = "{\"voltage\":\"%.2f\", \"current\":\"%.2f\", \"frequency\":\"%.2f\", \"device\":\"%s\", \"brewing\":\"%s\"}";
static const char cstrDeviceTelemetryProductJson[] = "{\"voltage\":\"%.2f\", \"current\":\"%.2f\", \"frequency\":\"%.2f\", \"device\":\"%s\", \"product\":\"%d\", \"size\":\"%d\"}";

/****************************************************************************************
 * Bindings
 ****************************************************************************************/

// Device Twins
static DX_DEVICE_TWIN_BINDING dt_maxD2CMessageTime = {.propertyName = "maxD2CMessageTime", 
                                                      .twinType = DX_DEVICE_TWIN_FLOAT, 
                                                      .handler = dt_generic_float_handler, 
                                                      &fMaxTimeBetweenD2CMessages};
static DX_DEVICE_TWIN_BINDING dt_smallMin = {.propertyName = "smallMin", 
                                             .twinType = DX_DEVICE_TWIN_FLOAT, 
                                             .handler = dt_generic_float_handler, 
                                             .context = &productArray[0].minRunTime};
static DX_DEVICE_TWIN_BINDING dt_smallMax = {.propertyName = "smallMax", 
                                             .twinType = DX_DEVICE_TWIN_FLOAT, 
                                             .handler = dt_generic_float_handler, 
                                             .context = &productArray[0].maxRunTime};
static DX_DEVICE_TWIN_BINDING dt_mediumMin = {.propertyName = "mediumMin", 
                                              .twinType = DX_DEVICE_TWIN_FLOAT, 
                                              .handler = dt_generic_float_handler, 
                                              .context = &productArray[1].minRunTime};
static DX_DEVICE_TWIN_BINDING dt_mediumMax = {.propertyName = "mediumMax", 
                                              .twinType = DX_DEVICE_TWIN_FLOAT, 
                                              .handler = dt_generic_float_handler, 
                                              .context = &productArray[1].maxRunTime};
static DX_DEVICE_TWIN_BINDING dt_largeMin = {.propertyName = "largeMin", 
                                             .twinType = DX_DEVICE_TWIN_FLOAT, 
                                             .handler = dt_generic_float_handler, 
                                             .context = &productArray[2].minRunTime};
static DX_DEVICE_TWIN_BINDING dt_largeMax = {.propertyName = "largeMax", 
                                             .twinType = DX_DEVICE_TWIN_FLOAT, 
                                             .handler = dt_generic_float_handler, 
                                             .context = &productArray[2].maxRunTime};
static DX_DEVICE_TWIN_BINDING dt_minCurrentThreshold = {.propertyName = "minCurrentThreshold", 
                                                        .twinType = DX_DEVICE_TWIN_FLOAT, 
                                                        .handler = dt_generic_float_handler, 
                                                        .context = &fMinCurrentThreshold};

// GPIO Bindings

static DX_GPIO_BINDING red_led = {.pin = SAMPLE_RGBLED_RED, 
                                   .name = "Red LED", 
                                   .direction = DX_OUTPUT, 
                                   .initialState = GPIO_Value_Low, 
                                   .invertPin = true};
                                   
static DX_GPIO_BINDING green_led = {.pin = SAMPLE_RGBLED_GREEN, 
                                   .name = "Green LED", 
                                   .direction = DX_OUTPUT, 
                                   .initialState = GPIO_Value_Low, 
                                   .invertPin = true};
static DX_GPIO_BINDING blue_led = {.pin = SAMPLE_RGBLED_BLUE, 
                                   .name = "Blue LED", 
                                   .direction = DX_OUTPUT, 
                                   .initialState = GPIO_Value_Low, 
                                   .invertPin = true};

// Timer Bindings
static DX_TIMER_BINDING tmr_update_network_led = {.period = {2, 0}, 
                                              .name = "tmr_update_network_led", 
                                              .handler = update_network_led_handler};

static DX_TIMER_BINDING tmr_get_current_data = {.period = {1, 0},
                                              .name = "tmr_get_current_data", 
                                              .handler = tmr_get_current_data_handler};

/****************************************************************************************
 * Inter Core Binding for PWR monitor RTApp
 *****************************************************************************************/
DX_INTERCORE_BINDING intercore_pwr_meter_binding = {
.sockFd = -1,
.nonblocking_io = true,
.rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
.interCoreCallback = receive_msg_handler,
.intercore_recv_block = &ic_rx_block_sample,
.intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_PWR_METER_RT_TO_HL)};

/****************************************************************************************
 * Binding sets
 ****************************************************************************************/

DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_maxD2CMessageTime, &dt_smallMin, &dt_smallMax, &dt_mediumMin, &dt_mediumMax,
                                             &dt_largeMin, &dt_largeMax, &dt_minCurrentThreshold};

DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {&red_led, &green_led, &blue_led};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_update_network_led, &tmr_get_current_data};