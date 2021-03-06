
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
#include "dx_uart.h"
#include <applibs/log.h>
#include <applibs/applications.h>
#include "rsl10.h"
#include "build_options.h"
#ifdef USE_IOT_CONNECT
#include "dx_avnet_iot_connect.h"
#endif // USE_IOT_CONNECT

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

#ifdef USE_WEB_PROXY
DX_PROXY_PROPERTIES proxy = {.proxyAddress = PROXY_ADDRESS,
                             .proxyPort = PROXY_PORT,
                             .proxyUsername = PROXY_USERNAME,
                             .proxyPassword = PROXY_PASSWORD,
                             .noProxyAdresses = NO_PROXY_ADDRESSES
};
#endif // USE_WEB_PROXY

// Enumeration to pass network status to setConectionStatusLed()
typedef enum {
	RGB_Invalid  = 0,
    RGB_No_Network = 1,        // No network connection
    RGB_Network_Connected = 2, // Connected to Azure, not authenticated
    RGB_IoT_Hub_Connected = 3, // Connected and authenticated to IoT Hub

} RGB_Status;

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/
// Declare device twin handlers
static DX_DECLARE_DEVICE_TWIN_HANDLER(rsl10AuthorizedDTFunction);
static DX_DECLARE_DEVICE_TWIN_HANDLER(enableOnboardingDTFunction);
static DX_DECLARE_DEVICE_TWIN_HANDLER(telemetryTimerDTFunction);

// Declare timer handlers
static DX_DECLARE_TIMER_HANDLER(send_telemetry_handler);
static DX_DECLARE_TIMER_HANDLER(update_network_led_handler);

// Uart handler
static void uartEventHandler(DX_UART_BINDING *uartBinding);

// RGB Network LED
static void setConnectionStatusLed(RGB_Status newNetworkStatus);

/****************************************************************************************
 * Bindings
 ****************************************************************************************/

// Device Twin Bindings
static DX_DEVICE_TWIN_BINDING dt_inside_rsl10 = {.propertyName = "insideMac",
                                                  .twinType = DX_DEVICE_TWIN_STRING,
                                                  .context = &Rsl10DeviceList[0],  // Address of first RSL10 structure
                                                  .handler = rsl10AuthorizedDTFunction}; 

static DX_DEVICE_TWIN_BINDING dt_outside_rsl10 = {.propertyName = "outsideMac",
                                                  .twinType = DX_DEVICE_TWIN_STRING,
                                                  .context = &Rsl10DeviceList[1],  // Address of second RSL10 structure
                                                  .handler = rsl10AuthorizedDTFunction}; 

static DX_DEVICE_TWIN_BINDING dt_enable_onboarding_rsl10 = {.propertyName = "enableRSL10Onboarding",
                                                  .twinType = DX_DEVICE_TWIN_BOOL,
                                                  .handler = enableOnboardingDTFunction}; 

static DX_DEVICE_TWIN_BINDING dt_telemetry_polltime = {.propertyName = "telemetryPollPeriod",
                                                  .twinType = DX_DEVICE_TWIN_INT,
                                                  .handler = telemetryTimerDTFunction}; 

// Timer Bindings
static DX_TIMER_BINDING tmr_send_telemetry = {.period = {TELEMETRY_SEND_PERIOD_SECONDS, 0}, 
                                              .name = "tmr_send_telemetry", 
                                              .handler = send_telemetry_handler};

static DX_TIMER_BINDING tmr_update_network_led = {.period = {2, 0}, 
                                              .name = "tmr_send_telemetry", 
                                              .handler = update_network_led_handler};

/****************************************************************************************
 * UART Peripherals
 ****************************************************************************************/
static DX_UART_BINDING pmodUart = {.uart = SAMPLE_PMOD_UART,
                                   .name = "PMOD UART",
                                   .handler = uartEventHandler,
                                   .uartConfig.baudRate = 115200,
                                   .uartConfig.dataBits = UART_DataBits_Eight,
                                   .uartConfig.parity = UART_Parity_None,
                                   .uartConfig.stopBits = UART_StopBits_One,
                                   .uartConfig.flowControl = UART_FlowControl_None};

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

/****************************************************************************************
 * Binding sets
 ****************************************************************************************/

DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_inside_rsl10, 
                                                  &dt_outside_rsl10, 
                                                  &dt_enable_onboarding_rsl10 , 
                                                  &dt_telemetry_polltime};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {&red_led, &green_led, &blue_led};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_send_telemetry, &tmr_update_network_led};
DX_UART_BINDING *uart_bindings[] = {&pmodUart};  
