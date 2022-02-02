#pragma once

#include "hw/azure_sphere_learning_path.h" // Hardware definition

#include "app_exit_codes.h"
#include "dx_gpio.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <applibs/log.h>
#include <applibs/storage.h>

#include "littlefs_mgr.h"

char writeMessage[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua\r\n";

// Forward declarations
static DX_DECLARE_TIMER_HANDLER(ButtonPressCheckHandler);

// The Project is configured for 64K of Mutable Storage (256 blocks * 256 block size)
#define BLOCK_SIZE 256
#define TOTAL_BLOCKS 256

int mutableStorageFd = -1;
lfs_file_t datafile;
lfs_t lfs;

const struct lfs_config g_littlefs_config = {.read = storage_read,
                                             .prog = storage_write,
                                             .erase = storage_erase,
                                             .sync = storage_sync,
                                             .read_size = BLOCK_SIZE,
                                             .prog_size = BLOCK_SIZE,
                                             .block_size = BLOCK_SIZE,
                                             .block_count = TOTAL_BLOCKS,
                                             .block_cycles = 1000,
                                             .cache_size = BLOCK_SIZE,
                                             .lookahead_size = BLOCK_SIZE,
                                             .name_max = 255};

/****************************************************************************************
 * GPIO Peripherals
 ****************************************************************************************/
static DX_GPIO_BINDING button_a = {.pin = BUTTON_A, .name = "button_a", .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW};

// All GPIOs added to gpio_bindings will be opened in InitPeripheralsAndHandlers
DX_GPIO_BINDING *gpio_bindings[] = {&button_a};

/****************************************************************************************
 * Timer Bindings
 ****************************************************************************************/
static DX_TIMER_BINDING buttonPressCheckTimer = {.period = {0, 1 * ONE_MS}, .name = "buttonPressCheckTimer", .handler = ButtonPressCheckHandler};

// All timers referenced in timers with be opened in the InitPeripheralsAndHandlers function
DX_TIMER_BINDING *timer_bindings[] = {&buttonPressCheckTimer};