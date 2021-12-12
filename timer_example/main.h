#pragma once

#include "hw/azure_sphere_learning_path.h" // Hardware definition

#include "app_exit_codes.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <applibs/log.h>

static void PeriodicHandler(EventLoopTimer *eventLoopTimer);
static void oneShotHandler(EventLoopTimer *eventLoopTimer);

/****************************************************************************************
 * Timer Bindings
 ****************************************************************************************/
static DX_TIMER_BINDING periodicTimer = {
    .repeat = &(struct timespec){6, 0}, .name = "periodicTimer", .handler = PeriodicHandler};

// A timer with no period specified or a zero period are initialized as disarmed oneshot timers
// A timer initialized with a delay will fire once with the delay expires
static DX_TIMER_BINDING oneShotTimer = {.delay = &(struct timespec){2, 500 * ONE_MS},
                                        .name = "oneShotTimer",
                                        .handler = oneShotHandler};

// All timers referenced in timers with be opened in the InitPeripheralsAndHandlers function
DX_TIMER_BINDING *timers[] = {&periodicTimer, &oneShotTimer};
