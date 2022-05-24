
#include "hw/sample_appliance.h" // Hardware definition
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
#include <applibs/log.h>
#include <applibs/applications.h>
#include <applibs/storage.h>
#include <errno.h>

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
typedef struct
{
    int shelfHeight_mm;
    int productHeight_mm;
    int productReserve;
    int currentProductCount;
    int lastProductCount;
    char* name;
} productShelf_t;

typedef struct
{
    productShelf_t shelf1Copy;
    productShelf_t shelf2Copy;
    bool lowPowerModeEnabled;
    int sleepTime;
} persistantMemory_t;

#define MIN_SHELF_HEIGHT_MM 100
#define MAX_SHELF_HEIGHT_MM 200

#define MIN_PRODUCT_HEIGHT_MM 20
#define MAX_PRODUCT_HEIGHT_MM 50

#define MIN_PRODUCT_RESERVE 1
#define MAX_PRODUCT_RESERVE 10

#define MIN_SLEEP_PERIOD 60*5       // 5 Minutes
#define MAX_SLEEP_PERIOD (60*60*12) // 12 Hours

/****************************************************************************************
 * Global Variables
 ****************************************************************************************/
int lowPowerSleepTime = 3600; // 1 hour
bool lowPowerEnabled = false;

productShelf_t productShelf1 = {.name = "Shelf 1",
                                .productHeight_mm = 32,
                                .productReserve = 1,
                                .lastProductCount = -1,
                                .shelfHeight_mm = 150};
productShelf_t productShelf2 = {.name = "Shelf 2",
                                .productHeight_mm = 32,
                                .productReserve = 1,
                                .lastProductCount = -1,
                                .shelfHeight_mm = 150};

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_low_power_mode_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_low_power_sleep_period_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_product_height_handler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dt_product_reserve_handler);

bool read_config_from_mutable_storage(persistantMemory_t* persistantConfig);
bool write_config_to_mutable_storage(void);
void update_config_from_mutable_storage(void);
bool updateConfigInMutableStorage(void);
void printConfig(void);

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

static DX_DEVICE_TWIN_BINDING dt_measured_shelf_height = {.propertyName = "MeasuredEmptyShelfHeight",
                                                        .twinType = DX_DEVICE_TWIN_INT};

/****************************************************************************************
 * Binding sets
 ****************************************************************************************/
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_low_power_mode, &dt_low_power_sleep_period,
                                                &dt_product_height_shelf1, &dt_product_height_shelf2, 
                                                &dt_product_reserve_shelf1, &dt_product_reserve_shelf2, 
                                                &dt_measured_shelf_height};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {};
DX_TIMER_BINDING *timer_bindings[] = {};
