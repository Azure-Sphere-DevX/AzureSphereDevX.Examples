#pragma once

#include "hw/sample_appliance.h"
#include "app_exit_codes.h"
#include "dx_azure_iot.h"
#include "dx_config.h"
#include "dx_json_serializer.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_intercore.h"
#include "dx_version.h"
#include <applibs/log.h>
#include <applibs/storage.h>
#include "lightranger5_click.h"


// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID ""
#define NETWORK_INTERFACE "wlan0"
#define SAMPLE_VERSION_NUMBER "1.0"

// Forward declarations
static DX_DECLARE_TIMER_HANDLER(read_range_handler);
static DX_DECLARE_TIMER_HANDLER(drive_leds_handler);
static DX_DECLARE_TIMER_HANDLER(ButtonPressCheckHandler);

static void receive_msg_handler(void *data_block, ssize_t message_length);

typedef enum
{
	LED_RANGE_NO_TARGET = 0,
    LED_RANGE_FAR,
    LED_RANGE_MID,
    LED_RANGE_CLOSE,
    LED_RANGE_STOP,
    LED_RANGE_UNDEFINED = (-1)
} led_range_t;
static led_range_t ledStatus = LED_RANGE_UNDEFINED;

#define MAXIUM_RANGE 2500 // Lightranger5 max range
static int targetRange = 2000; // 2 meters
static int currentRange = -1;
static int midRange = 2000 + 332;  // targetRange + 2 * ((MAXIUM_RANGE - targetRange) / 3);
static int closeRange = 2000 + 167;      // targetRange + 1 * ((MAXIUM_RANGE - targetRange) / 3);

typedef enum
{
	LED_FLASH_RATE_FAR = 500,   // ms
    LED_FLASH_RATE_MID = 250,   // ms
    LED_FLASH_RATE_IDLE = 250,
    LED_FLASH_RATE_CLOSE = 125,
    LED_FLASH_RATE_UNDEFINED = (-1)
} led_flash_rate_t;

DX_USER_CONFIG dx_config;

// Inter core message blocks
IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_HL_TO_RT ic_tx_block;
IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_RT_TO_HL ic_rx_block;

bool initPersistantMemory(int *targetRangeMm);
bool readTargetRangeFromMutableStorage(int *targetRangeVar);
bool writeTargetRangeToMutableStorage(int targetRangeVar);

DX_INTERCORE_BINDING intercore_LIGHTRANGER5_click_binding = {
    .sockFd = -1,
    .nonblocking_io = true,
    .rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
    .interCoreCallback = receive_msg_handler,
    .intercore_recv_block = &ic_rx_block,
    .intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_LIGHTRANGER5_CLICK_RT_TO_HL)};

// Timer bindings
static DX_TIMER_BINDING tmrReadRange = {.period = {0, 250*ONE_MS}, .name = "tmrReadRange", .handler = read_range_handler};
static DX_TIMER_BINDING tmrDriveLeds = {.period = {5, 0}, .name = "tmrDriveLeds", .handler = drive_leds_handler};
static DX_TIMER_BINDING buttonPressCheckTimer = {.period = {0, ONE_MS*10}, .name = "buttonPressCheckTimer", .handler = ButtonPressCheckHandler};

/****************************************************************************************
 * GPIO Peripherals
 ****************************************************************************************/
static DX_GPIO_BINDING buttonA =      {.pin = SAMPLE_BUTTON_1,     .name = "buttonA",      .direction = DX_INPUT,   .detect = DX_GPIO_DETECT_LOW};
static DX_GPIO_BINDING buttonB =      {.pin = SAMPLE_BUTTON_2,     .name = "buttonB",      .direction = DX_INPUT,   .detect = DX_GPIO_DETECT_LOW};
static DX_GPIO_BINDING userLedRed =   {.pin = SAMPLE_RGBLED_RED,   .name = "userLedRed",   .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING userLedGreen = {.pin = SAMPLE_RGBLED_GREEN, .name = "userLedGreen", .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING userLedBlue =  {.pin = SAMPLE_RGBLED_BLUE,  .name = "userLedBlue",  .direction = DX_OUTPUT,  .initialState = GPIO_Value_Low, .invertPin = true};

// All bindings referenced in the folowing binding sets are initialised in the InitPeripheralsAndHandlers function
DX_GPIO_BINDING *gpio_bindings[] = {&buttonA, &buttonB, &userLedRed, &userLedGreen, &userLedBlue};
DX_TIMER_BINDING *timer_bindings[] = {&tmrReadRange, &tmrDriveLeds, &buttonPressCheckTimer};