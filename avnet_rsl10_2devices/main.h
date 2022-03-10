
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
#define DEFAULT_TELEMETRY_TX_TIME 15

DX_PROXY_PROPERTIES proxy = {.proxyAddress = "192.168.8.2",
                             .proxyPort = 3128,
                             .proxyUsername = NULL,
                             .proxyPassword = NULL,
                             .noProxyAdresses = NULL};

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/
// Declare device twin handlers
static DX_DECLARE_DEVICE_TWIN_HANDLER(rsl10AuthorizedDTFunction);
static DX_DECLARE_DEVICE_TWIN_HANDLER(enableOnboardingDTFunction);
static DX_DECLARE_DEVICE_TWIN_HANDLER(telemetryTimerDTFunction);

// Declare timer handlers
static DX_DECLARE_TIMER_HANDLER(send_telemetry_handler);

// Uart handler
static void uartEventHandler(DX_UART_BINDING *uartBinding);

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
static DX_TIMER_BINDING tmr_send_telemetry = {.period = {DEFAULT_TELEMETRY_TX_TIME, 0}, 
                                              .name = "tmr_send_telemetry", 
                                              .handler = send_telemetry_handler};

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

//static DX_DEVICE_TWIN_BINDING dt_desired_sample_rate = {.propertyName = "DesiredSampleRate", .twinType = DX_DEVICE_TWIN_INT, .handler = dt_desired_sample_rate_handler};
//static DX_GPIO_BINDING gpio_led = {.pin = LED2, .name = "gpio_led", .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true};


/****************************************************************************************
 * Binding sets
 ****************************************************************************************/
// TODO: Update each binding set below with the bindings defined above.  Add bindings by reference, i.e., &dt_desired_sample_rate
// These sets are used by the initailization code.

DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_inside_rsl10, 
                                                  &dt_outside_rsl10, 
                                                  &dt_enable_onboarding_rsl10 , 
                                                  &dt_telemetry_polltime};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_send_telemetry};
DX_UART_BINDING *uart_bindings[] = {&pmodUart};  
