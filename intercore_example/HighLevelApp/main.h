#pragma once

// Hardware definition
#include "hw/azure_sphere_learning_path.h"

// Learning Path Libraries
#include "dx_config.h"
#include "app_exit_codes.h"
#include "dx_intercore.h"
#include "dx_terminate.h"
#include "dx_timer.h"

#include "../IntercoreContract/intercore_contract.h"

// System Libraries
#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/log.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define REAL_TIME_COMPONENT_ID_ASYNCHRONOUS "09F0654C-674D-4C93-A4EA-DA60ACD4FD32"
#define REAL_TIME_COMPONENT_ID_SYNCHRONOUS "5DFA4F92-B474-44EC-BF99-DDBC5B91F795"

// Forward signatures
static void IntercoreResponseHandler(void *data_block, ssize_t message_length);
static void IntercoreAsynchronousHandler(EventLoopTimer *eventLoopTimer);
static void IntercoreSynchronousHandler(EventLoopTimer *eventLoopTimer);

INTER_CORE_BLOCK ic_block_asyncronous = {.cmd = IC_UNKNOWN, .msgId = 0, .message = {0}};
INTER_CORE_BLOCK ic_block_synchronous = {.cmd = IC_UNKNOWN, .msgId = 0, .message = {0}};

DX_INTERCORE_BINDING intercore_app_asynchronous = {
    .sockFd = -1,
    .nonblocking_io = true,
    .rtAppComponentId = REAL_TIME_COMPONENT_ID_ASYNCHRONOUS,
    .interCoreCallback = IntercoreResponseHandler,
    .intercore_recv_block = &ic_block_asyncronous,
    .intercore_recv_block_length = sizeof(ic_block_asyncronous)};

DX_INTERCORE_BINDING intercore_app_sychronous = {
    .sockFd = -1,
    .nonblocking_io = true,
    .rtAppComponentId = REAL_TIME_COMPONENT_ID_SYNCHRONOUS,
    .interCoreCallback = NULL,
    .intercore_recv_block = &ic_block_synchronous,
    .intercore_recv_block_length = sizeof(ic_block_synchronous)};

// Timers
static DX_TIMER_BINDING intercoreAsynchronousExampleTimer = {
    .period = {1, 0},
    .name = "intercoreAsynchronousExampleTimer",
    .handler = IntercoreAsynchronousHandler};

static DX_TIMER_BINDING intercoreSynchronousExampleTimer = {
    .period = {1, 0},
    .name = "intercoreSynchronousExampleTimer",
    .handler = IntercoreSynchronousHandler};

DX_TIMER_BINDING *timerSet[] = {&intercoreAsynchronousExampleTimer,
                                &intercoreSynchronousExampleTimer};