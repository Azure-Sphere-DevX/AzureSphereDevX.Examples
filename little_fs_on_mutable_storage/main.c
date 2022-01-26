/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 *
 * This example is built on the Azure Sphere DevX library.
 *   1. DevX is an Open Source community-maintained implementation of the Azure Sphere SDK samples.
 *   2. DevX is a modular library that simplifies common development scenarios.
 *        - You can focus on your solution, not the plumbing.
 *   3. DevX documentation is maintained at https://github.com/gloveboxes/AzureSphereDevX/wiki
 *	 4. The DevX library is not a substitute for understanding the Azure Sphere SDK Samples.
 *          - https://github.com/Azure/azure-sphere-samples
 *
 * DEVELOPER BOARD SELECTION
 *
 * The following developer boards are supported.
 *
 *	 1. AVNET Azure Sphere Starter Kit.
 *   2. AVNET Azure Sphere Starter Kit Revision 2.
 *	 3. Seeed Studio Azure Sphere MT3620 Development Kit aka Reference Design Board or rdb.
 *	 4. Seeed Studio Seeed Studio MT3620 Mini Dev Board.
 *
 * ENABLE YOUR DEVELOPER BOARD
 *
 * Each Azure Sphere developer board manufacturer maps pins differently. You need to select the
 *    configuration that matches your board.
 *
 * Follow these steps:
 *
 *	 1. Open CMakeLists.txt.
 *	 2. Uncomment the set command that matches your developer board.
 *	 3. Click File, then Save to auto-generate the CMake Cache.
 *
 ************************************************************************************************/

#include "main.h"


static void write_little_fs(void)
{
    // Create a file
    Log_Debug("Create File /data/lorem.txt\n");
    if (lfs_file_open(&lfs, &datafile, "/data/lorem.txt", LFS_O_RDWR | LFS_O_CREAT) != LFS_ERR_OK) {
        return;
    }

    size_t dataLength = strlen(writeMessage);

    Log_Debug("Write to file: %s\n", writeMessage);

    for (size_t i = 0; i < 25; i++) {
        if (lfs_file_write(&lfs, &datafile, writeMessage, dataLength) != dataLength) {
            break;
        }
    }    

    lfs_file_sync(&lfs, &datafile);

    // Close the file
    Log_Debug("Close file\n");
    lfs_file_close(&lfs, &datafile);
}

static void read_little_fs(void) {

    size_t dataLength = strlen(writeMessage);

    if (lfs_file_open(&lfs, &datafile, "/data/lorem.txt", LFS_O_RDWR) != LFS_ERR_OK) {
        return;
    }

    // Read from the file
    char buffer[dataLength + 1];

    for (size_t i = 0; i < 25; i++) {
        memset(buffer, 0x00, dataLength + 1);
        if (lfs_file_read(&lfs, &datafile, buffer, dataLength) == dataLength) {
            Log_Debug("Read data: %s\n", buffer);
        }
    }

    // Close the file
    Log_Debug("Close file\n");
    lfs_file_close(&lfs, &datafile) == LFS_ERR_OK;
}

/// <summary>
/// Handler to check for Button Presses
/// </summary>
static DX_TIMER_HANDLER(ButtonPressCheckHandler)
{
    static GPIO_Value_Type button_a_state;
    static bool operation_select = true;

    if (dx_gpioStateGet(&button_a, &button_a_state)) {

        if (operation_select) {
            write_little_fs();
        } else {
            read_little_fs();
        }

        operation_select = !operation_select;
    }
}
DX_TIMER_HANDLER_END


static void init_little_fs(void)
{
    if (lfs_mount(&lfs, &g_littlefs_config) != LFS_ERR_OK) {
        if (lfs_format(&lfs, &g_littlefs_config) != LFS_ERR_OK) {
            dx_terminate(LITTLE_FS_FORMAT_FAIL);
        }

        if (lfs_mount(&lfs, &g_littlefs_config) != LFS_ERR_OK) {
            dx_terminate(LITTLE_FS_MOUNT_FAIL);
        }
    }

    Log_Debug("Create Directory '/data'\n");
    
    int result = lfs_mkdir(&lfs, "/data");

    if (!(result != LFS_ERR_OK || result != LFS_ERR_EXIST)) {
        dx_terminate(LITTLE_FS_MKDIR_FAIL);
    }
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timers.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));

    mutableStorageFd = Storage_OpenMutableFile();
    init_little_fs();
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
    dx_gpioSetClose(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerEventLoopStop();
}

int main(void)
{
    dx_registerTerminationHandler();
    InitPeripheralsAndHandlers();

    // Main loop
    while (!dx_isTerminationRequired()) {
        int result = EventLoop_Run(dx_timerGetEventLoop(), -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == -1 && errno != EINTR) {
            dx_terminate(DX_ExitCode_Main_EventLoopFail);
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return dx_getTerminationExitCode();
}