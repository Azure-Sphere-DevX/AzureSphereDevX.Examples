#pragma once

#include "hw/azure_sphere_learning_path.h" // Hardware definition

#include "app_exit_codes.h"
#include "dx_uart.h"
#include "dx_gpio.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <applibs/log.h>

// Forward declarations
static DX_DECLARE_TIMER_HANDLER(ButtonPressCheckHandler);
static void uart_rx_handler1(DX_UART_BINDING *uartBinding);

/****************************************************************************************
 * GPIO Peripherals
 ****************************************************************************************/
static DX_GPIO_BINDING buttonA = {
    .pin = BUTTON_A, .name = "buttonA", .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW};

static DX_GPIO_BINDING buttonB = {
    .pin = BUTTON_B, .name = "buttonB", .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW};

// All GPIOs added to gpio_bindings will be opened in InitPeripheralsAndHandlers
DX_GPIO_BINDING *gpio_bindings[] = {&buttonA, &buttonB};

/****************************************************************************************
 * UART Peripherals
 ****************************************************************************************/
static DX_UART_BINDING loopBackClick1 = {.uart = UART_CLICK1,
                                         .name = "uart click1",
                                         .handler = uart_rx_handler1,
                                         .uartConfig.baudRate = 115200,
                                         .uartConfig.dataBits = UART_DataBits_Eight,
                                         .uartConfig.parity = UART_Parity_None,
                                         .uartConfig.stopBits = UART_StopBits_One,
                                         .uartConfig.flowControl = UART_FlowControl_None};

// All UARTSs added to uart_bindings will be opened in InitPeripheralsAndHandlers
DX_UART_BINDING *uart_bindings[] = {&loopBackClick1};

/****************************************************************************************
 * Timer Bindings
 ****************************************************************************************/
static DX_TIMER_BINDING buttonPressCheckTimer = {
    .period = {0, ONE_MS}, .name = "buttonPressCheckTimer", .handler = ButtonPressCheckHandler};

// All timers referenced in timer_bindings with be opened in the InitPeripheralsAndHandlers function
DX_TIMER_BINDING *timer_bindings[] = {&buttonPressCheckTimer};
