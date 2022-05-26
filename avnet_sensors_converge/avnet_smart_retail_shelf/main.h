#pragma once

#include "hw/sample_appliance.h" // Hardware definition
#include "dx_gpio.h"
#include "app_exit_codes.h"
#include "dx_azure_iot.h"
#include "dx_avnet_iot_connect.h"
#include "dx_config.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_direct_methods.h"
#include "dx_version.h"
#include "dx_timer.h"
#include "dx_intercore.h"
#include <applibs/log.h>
#include "dx_deferred_update.h"
#include <applibs/applications.h>
#include <applibs/storage.h>
#include <errno.h>
#include "persistantConfig.h"
#include "avnetSmartShelfInterface.h"

// Define all your application definitions, message properties/contentProperties,
// bindings and binding sets.

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "" 

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

/****************************************************************************************
 * Application defines
 ****************************************************************************************/

#define SENSOR_READ_PERIOD_SECONDS 10

/****************************************************************************************
 * Global Variables
 ****************************************************************************************/
int lowPowerSleepTime = 3600; // 1 hour
bool lowPowerEnabled = false;

typedef struct
{
    float temp;
    float hum;
    float pressure;
} phtData_t;

phtData_t pht = {.hum = -1,
                 .temp = -1,
                 .pressure = -1};

productShelf_t productShelf1 = {.name = "StockLevelShelf1",
                                .alertName = "LowStockAlertShelf1",
                                .productHeight_mm = 33,
                                .productReserve = -1,
                                .lastProductCount = -1160,
                                .shelfHeight_mm = 160,
                                .stockLevelAlertSent = false};
productShelf_t productShelf2 = {.name = "StockLevelShelf2",
                                .alertName = "LowStockAlertShelf2",
                                .productHeight_mm = 33,
                                .productReserve = -1,
                                .lastProductCount = -1,
                                .shelfHeight_mm = 160,
                                .stockLevelAlertSent = false};

#define STOCK_HISTORY_DEPTH 4
#define STOCK_HISTORY_DEPTH_MASK 0x03

typedef struct
{
    int historyIndex;
    int historyArray[STOCK_HISTORY_DEPTH];
} stockHistory_t;

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/

static int calculateStockLevel(productShelf_t* shelf, int range_mm);
bool checkShelfStock(productShelf_t* shelfData, stockHistory_t* shelfHistory);
void checkStockReserveLevel(productShelf_t* shelfData, char* lowStockTelemetryKey);

/****************************************************************************************
 * Device Twins
 ****************************************************************************************/
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_low_power_mode_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_low_power_sleep_period_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_product_height_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_product_reserve_handler);

/****************************************************************************************
 * Timers
 ****************************************************************************************/
static DX_DECLARE_TIMER_HANDLER(ButtonPressCheckHandler);
static DX_DECLARE_TIMER_HANDLER(read_sensors_handler);
static DX_DECLARE_TIMER_HANDLER(send_telemetry_handler);
static DX_DECLARE_TIMER_HANDLER(shelf_stock_check_handler);

void printConfig(void);
//static void receive_msg_handler(void *data_block, ssize_t message_length);

IC_COMMAND_BLOCK_SMART_SHELF_HL_TO_RT ic_tx_block;
IC_COMMAND_BLOCK_SMART_SHELF_RT_TO_HL ic_rx_block;

/****************************************************************************************
 * Telemetry message buffer property sets
 ****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
#define JSON_MESSAGE_BYTES 256
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};

static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

/****************************************************************************************
 * Bindings
 ****************************************************************************************/

/****************************************************************************************
 * Device Twins
 ****************************************************************************************/
static DX_DEVICE_TWIN_BINDING dt_low_power_mode = {.propertyName = "LowPowerModeEnabled",
                                                        .twinType = DX_DEVICE_TWIN_BOOL,
                                                        .handler = dt_low_power_mode_handler};

static DX_DEVICE_TWIN_BINDING dt_low_power_sleep_period = {.propertyName = "lowPowerSleepPeriod",
                                                        .twinType = DX_DEVICE_TWIN_INT,
                                                        .handler = dt_low_power_sleep_period_handler};

static DX_DEVICE_TWIN_BINDING dt_product_height_shelf1 = {.propertyName = "ProductHeightShelf1",
                                                        .twinType = DX_DEVICE_TWIN_INT,
                                                        .handler = dt_product_height_handler,
                                                        .context = &productShelf1};

static DX_DEVICE_TWIN_BINDING dt_product_height_shelf2 = {.propertyName = "ProductHeightShelf2",
                                                        .twinType = DX_DEVICE_TWIN_INT,
                                                        .handler = dt_product_height_handler,
                                                        .context = &productShelf2};

static DX_DEVICE_TWIN_BINDING dt_product_reserve_shelf1 = {.propertyName = "ProductReserveShelf1",
                                                        .twinType = DX_DEVICE_TWIN_INT,
                                                        .handler = dt_product_reserve_handler,
                                                        .context = &productShelf1};

static DX_DEVICE_TWIN_BINDING dt_product_reserve_shelf2 = {.propertyName = "ProductReserveShelf2",
                                                        .twinType = DX_DEVICE_TWIN_INT,
                                                        .handler = dt_product_reserve_handler,
                                                        .context = &productShelf2};

static DX_DEVICE_TWIN_BINDING dt_measured_shelf1_height = {.propertyName = "MeasuredEmptyShelf1Height",
                                                        .twinType = DX_DEVICE_TWIN_INT};

static DX_DEVICE_TWIN_BINDING dt_measured_shelf2_height = {.propertyName = "MeasuredEmptyShelf2Height",
                                                        .twinType = DX_DEVICE_TWIN_INT};


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

/****************************************************************************************
 * Timers
 ****************************************************************************************/

static DX_TIMER_BINDING tmr_button_monitor = {.period = {0, ONE_MS*10}, 
                                              .name = "tmr_button_monitor", 
                                              .handler = ButtonPressCheckHandler};

static DX_TIMER_BINDING tmr_read_sensors = {.period = {1, 0}, 
                                            .name = "tmr_read_sensors", 
                                            .handler = read_sensors_handler};

static DX_TIMER_BINDING tmr_send_telemetry = {.delay = &(struct timespec){2, 500 * ONE_MS}, 
                                              .name = "tmr_send_telemetry", 
                                              .handler = send_telemetry_handler};

static DX_TIMER_BINDING tmr_check_shelf_stock = {.period = {1, 0}, 
                                            .name = "tmr_check_shelf_stock", 
                                            .handler = shelf_stock_check_handler};

/****************************************************************************************
 * Inter Core Bindings
 *****************************************************************************************/
DX_INTERCORE_BINDING intercore_smart_shelf_binding = {
    .sockFd = -1,
    .nonblocking_io = true,
    .rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
    .interCoreCallback = NULL,
    .intercore_recv_block = &ic_rx_block,
    .intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_SMART_SHELF_RT_TO_HL)
};

/****************************************************************************************
 * Binding sets
 ****************************************************************************************/
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_low_power_mode, 
                                                  &dt_low_power_sleep_period,
                                                  &dt_product_height_shelf1, 
                                                  &dt_product_height_shelf2, 
                                                  &dt_product_reserve_shelf1, 
                                                  &dt_product_reserve_shelf2, 
                                                  &dt_measured_shelf1_height, 
                                                  &dt_measured_shelf2_height};

DX_GPIO_BINDING *gpio_bindings[] = {&buttonA, 
                                    &buttonB, 
                                    &userLedRed, 
                                    &userLedGreen, 
                                    &userLedBlue, 
                                    &wifiLed, 
                                    &appLed};

DX_TIMER_BINDING *timer_bindings[] = {&tmr_button_monitor, 
                                      &tmr_read_sensors, 
                                      &tmr_send_telemetry, 
                                      &tmr_check_shelf_stock};
