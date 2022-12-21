
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

// PHT Click
#include "dx_intercore.h"
#include "pht_click.h"

// Avnet IoT Connect
#include "dx_avnet_iot_connect.h"

// Sleep
#include <applibs/powermanagement.h>

// Defer Update
#include "dx_deferred_update.h"

#include "dx_uart.h"

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
 * Application defines
 ****************************************************************************************/
// Define to send debug data out the UART in Click Socket #1.  This is usefull when unit
// testing the OTA update logic
//#define INCLUDE_OTA_DEBUG

// If the application will connect to Avnet's IoTConnect platform enable the 
// #define below.  Otherwise the application will try to connect to an Azure IoTHub.
#define USE_AVNET_IOTCONNECT

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/
static DX_DECLARE_TIMER_HANDLER(waitForConnectionHandler);
static DX_DECLARE_TIMER_HANDLER(waitToSleepHandler);
static DX_DECLARE_DEVICE_TWIN_HANDLER(dtSleepPeriodHandler);

// Memory blocks for intercore comms
IC_COMMAND_BLOCK_PHT_CLICK_HL_TO_RT ic_tx_block;
IC_COMMAND_BLOCK_PHT_CLICK_RT_TO_HL ic_rx_block;

/****************************************************************************************
 * Bindings
 ****************************************************************************************/

/****************************************************************************************
 * Inter Core Bindings
 *****************************************************************************************/
DX_INTERCORE_BINDING intercore_pht_click_binding = {
    .sockFd = -1,
    .nonblocking_io = true,
    .rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
    .interCoreCallback = NULL,
    .intercore_recv_block = &ic_rx_block,
    .intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_PHT_CLICK_RT_TO_HL)};

/****************************************************************************************
 * Timer Bindings
 ****************************************************************************************/
static DX_TIMER_BINDING connectionCheckTimer = {.period = {5, 0 * ONE_MS}, .name = "ConnectionCheckTimer", .handler = waitForConnectionHandler};
static DX_TIMER_BINDING sleepCheckTimer = {.period = {0, 0 * ONE_MS}, .name = "WaitToSleepTimer", .handler = waitToSleepHandler};

/****************************************************************************************
 * GPIO Bindings
 ****************************************************************************************/
static DX_GPIO_BINDING ledRed =   {.pin = SAMPLE_RGBLED_RED,   .name = "LedRed",   .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING ledGreen = {.pin = SAMPLE_RGBLED_GREEN, .name = "LedGreen", .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING ledBlue =  {.pin = SAMPLE_RGBLED_BLUE,  .name = "LedBlue",  .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};

/****************************************************************************************
 * Device Twin Bindings
 ****************************************************************************************/
static DX_DEVICE_TWIN_BINDING dt_sleep_period = {.propertyName = "sleepPeriodMinutes", .twinType = DX_DEVICE_TWIN_INT,  .handler = dtSleepPeriodHandler};	
static DX_DEVICE_TWIN_BINDING dt_version_string = {.propertyName = "versionString", .twinType = DX_DEVICE_TWIN_STRING,  .handler = NULL};	

/****************************************************************************************
 * UART Peripherals
 ****************************************************************************************/
#ifdef INCLUDE_OTA_DEBUG
static DX_UART_BINDING debugClick1 = {.uart = SAMPLE_UART_LOOPBACK,
                                         .name = "uart click1",
                                         .handler = NULL,
                                         .uartConfig.baudRate = 115200,
                                         .uartConfig.dataBits = UART_DataBits_Eight,
                                         .uartConfig.parity = UART_Parity_None,
                                         .uartConfig.stopBits = UART_StopBits_One,
                                         .uartConfig.flowControl = UART_FlowControl_None};

// All UARTSs added to uart_bindings will be opened in InitPeripheralsAndHandlers
DX_UART_BINDING *uart_bindings[] = {&debugClick1};
#endif 


/****************************************************************************************
 * Binding sets
 ****************************************************************************************/
DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_sleep_period, &dt_version_string};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {&ledRed, &ledGreen, &ledBlue};
DX_TIMER_BINDING *timer_bindings[] = {&connectionCheckTimer, &sleepCheckTimer};
