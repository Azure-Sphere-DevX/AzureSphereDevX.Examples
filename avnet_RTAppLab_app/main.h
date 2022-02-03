
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
#include "generic_rt_app.h"
#include "dx_intercore.h"

/****************************************************************************************
 * Forward declarations
 ****************************************************************************************/
static void IntercoreResponseHandler(void *data_block, ssize_t message_length);
static DX_DECLARE_TIMER_HANDLER(IntercoreSendLedUpdateHandler);
static DX_DECLARE_TIMER_HANDLER(IntercoreSendStringUpdateHandler);

/****************************************************************************************
 * InterCore TX/RX block structures
 ****************************************************************************************/
IC_COMMAND_BLOCK_LAB_CMDS_RT_TO_HL ic_rx_block;
IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT ic_tx_block;

/****************************************************************************************
 * Bindings
 ****************************************************************************************/
DX_INTERCORE_BINDING intercore_app_asynchronous = {.nonblocking_io = true,
                                                   .rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
                                                   .interCoreCallback = IntercoreResponseHandler,
                                                   .intercore_recv_block = &ic_rx_block,
                                                   .intercore_recv_block_length = sizeof(ic_rx_block)};

// Timers
static DX_TIMER_BINDING tmr_IntercoreSendLedUpdate = {.period = {1, 0}, .name = "IntercoreSendLedUpdateHandler", .handler = IntercoreSendLedUpdateHandler};
static DX_TIMER_BINDING tmr_IntercoreSendStringUpdate = {.period = {1, 0}, .name = "IntercoreSendStringUpdateHandler", .handler = IntercoreSendStringUpdateHandler};


/****************************************************************************************
 * Binding sets
 ****************************************************************************************/
// TODO: Update each binding set below with the bindings defined above.  Add bindings by reference, i.e., &dt_desired_sample_rate
// These sets are used by the initailization code.

DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {};
DX_DIRECT_METHOD_BINDING *direct_method_bindings[] = {};
DX_GPIO_BINDING *gpio_bindings[] = {};
DX_TIMER_BINDING *timer_bindings[] = {&tmr_IntercoreSendLedUpdate, &tmr_IntercoreSendStringUpdate};
