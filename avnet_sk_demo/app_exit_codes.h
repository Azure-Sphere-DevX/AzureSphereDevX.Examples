/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

#pragma once

// Include the DevX exit codes
#include "dx_exit_codes.h"

/// <summary>
/// Exit codes for this application.  Application exit codes 
/// must be between 1 and 149, where 0 is reserved for successful 
//  termination.  Exit codes 150 - 254 are reserved for the
//  Exit_Code enumeration located in dx_exit_codes.h.
/// </summary>
typedef enum {
    ExitCode_ConsumeEventLoopWifiMonitor = 1,
    ExitCode_ConsumeEventLoopReadSensors = 2,
    ExitCode_ConsumeEventLoopRestart     = 3,
    ExitCode_ConsumeEventButtonHandler   = 4,
    ExitCode_ReadButtonAError            = 5,
    ExitCode_ReadButtonBError            = 6,   
    ExitCode_ConsumeEventOledHandler     = 7,
    ExitCode_rtAppInitFailed             = 8 // Is the real time application sidloaded onto the device?
} App_Exit_Code;