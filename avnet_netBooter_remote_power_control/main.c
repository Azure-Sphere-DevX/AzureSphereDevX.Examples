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
 *  How to use this sample
 * 
 *    Developers can use this sample as a starting point for their DevX based Azure Sphere 
 *    application.  It will connect to an Azure IoTHub, IOTCentral or Avnet's IoTConnect.  
 * 
 *    There are sections marked with "TODO" that the developer can review for hints on where
 *    to add code, or to enable code that may be needed for general support, such as sending
 *    telemetry.
 * 
 ************************************************************************************************/
#include "main.h"
#include <applibs/networking.h>
#include <arpa/inet.h>

/****************************************************************************************
 * Implementation
 ****************************************************************************************/
// Variable to hold the devcie IP address
char deviceIpAddress[] = DEVICE_IP;

static bool isNetworkStackReady = false;
static struct in_addr localIpAddress;
static struct in_addr subnetMask;
static struct in_addr gatewayIpAddress;
static const char NetworkInterface[] = "eth0";

/// <summary>
///     Configure and start DHCP server on the specified network interface.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface on which to start the DHCP server.
/// </param>
/// <returns>0 on success, or -1 on failure</returns>
static int ConfigureAndStartDhcpSever(const char* interfaceName)
{
    Networking_DhcpServerConfig dhcpServerConfig;
    Networking_DhcpServerConfig_Init(&dhcpServerConfig);

    struct in_addr dhcpStartIpAddress;
    inet_aton(deviceIpAddress, &dhcpStartIpAddress);

    Networking_DhcpServerConfig_SetLease(&dhcpServerConfig, dhcpStartIpAddress, 1, subnetMask,
        gatewayIpAddress, 24);
    Networking_DhcpServerConfig_SetNtpServerAddresses(&dhcpServerConfig, &localIpAddress, 1);

    int result = Networking_DhcpServer_Start(interfaceName, &dhcpServerConfig);
    Networking_DhcpServerConfig_Destroy(&dhcpServerConfig);
    if (result != 0) {
        Log_Debug("ERROR: Networking_DhcpServer_Start: %d (%s)\n", errno, strerror(errno));
        return -1;
    }
    Log_Debug("INFO: DHCP server has started on network interface: %s.\n", interfaceName);
    return 0;
}

/// <summary>
///     Configure the specified network interface with a static IP address.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface to be configured.
/// </param>
/// <returns>0 on success, or -1 on failure</returns>
static int ConfigureNetworkInterfaceWithStaticIp(const char* interfaceName)
{
//    Log_Debug("ConfigureNetworkInterfaceWithStaticIp() Called\n");

    Networking_IpConfig ipConfig;
    Networking_IpConfig_Init(&ipConfig);
    inet_aton(MY_IP, &localIpAddress);
    inet_aton("255.255.0.0", &subnetMask);
    inet_aton("0.0.0.0", &gatewayIpAddress);
    Networking_IpConfig_EnableStaticIp(&ipConfig, localIpAddress, subnetMask,
        gatewayIpAddress);

    int result = Networking_IpConfig_Apply(interfaceName, &ipConfig);
    Networking_IpConfig_Destroy(&ipConfig);
    if (result != 0) {
        Log_Debug("ERROR: Networking_IpConfig_Apply: %d (%s)\n", errno, strerror(errno));
        return -1;
    }
    Log_Debug("INFO: Set static IP address on network interface: %s.\n", interfaceName);

    return 0;
}


/// <summary>
///     Check network status and display information about all available network interfaces.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int CheckNetworkStatus(void)
{

    Log_Debug("CheckNetworkStatus(void) called\n");
    // Ensure the necessary network interface is enabled.
    int result = Networking_SetInterfaceState(NetworkInterface, true);
    if (result != 0) {
        if (errno == EAGAIN) {
            Log_Debug("INFO: The networking stack isn't ready yet, will try again later.\n");
            return 0;
        }
        else {
            Log_Debug(
                "ERROR: Networking_SetInterfaceState for interface '%s' failed: errno=%d (%s)\n", NetworkInterface, errno, strerror(errno));
            return -1;
        }
    }
    isNetworkStackReady = true;

    // Display total number of network interfaces.
    ssize_t count = Networking_GetInterfaceCount();
    if (count == -1) {
        Log_Debug("ERROR: Networking_GetInterfaceCount: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }
    Log_Debug("INFO: Networking_GetInterfaceCount: count=%zd\n", count);

    // Read current status of all interfaces.
    size_t bytesRequired = ((size_t)count) * sizeof(Networking_NetworkInterface);
    Networking_NetworkInterface* interfaces = malloc(bytesRequired);
    if (!interfaces) {
        abort();
    }

    ssize_t actualCount = Networking_GetInterfaces(interfaces, (size_t)count);
    if (actualCount == -1) {
        Log_Debug("ERROR: Networking_GetInterfaces: errno=%d (%s)\n", errno, strerror(errno));
    }
    Log_Debug("INFO: Networking_GetInterfaces: actualCount=%zd\n", actualCount);

    // Print detailed description of each interface.
    for (ssize_t i = 0; i < actualCount; ++i) {
        Log_Debug("INFO: interface #%zd\n", i);

        // Print the interface's name.
        char printName[IF_NAMESIZE + 1];
        memcpy(printName, interfaces[i].interfaceName, sizeof(interfaces[i].interfaceName));
        printName[sizeof(interfaces[i].interfaceName)] = '\0';
        Log_Debug("INFO:   interfaceName=\"%s\"\n", interfaces[i].interfaceName);

        // Print whether the interface is enabled.
        Log_Debug("INFO:   isEnabled=\"%d\"\n", interfaces[i].isEnabled);

        // Print the interface's configuration type.
        Networking_IpType ipType = interfaces[i].ipConfigurationType;
        const char* typeText;
        switch (ipType) {
        case Networking_IpType_DhcpNone:
            typeText = "DhcpNone";
            break;
        case Networking_IpType_DhcpClient:
            typeText = "DhcpClient";
            break;
        default:
            typeText = "unknown-configuration-type";
            break;
        }
        Log_Debug("INFO:   ipConfigurationType=%d (%s)\n", ipType, typeText);

        // Print the interface's medium.
        Networking_InterfaceMedium_Type mediumType = interfaces[i].interfaceMediumType;
        const char* mediumText;
        switch (mediumType) {
        case Networking_InterfaceMedium_Unspecified:
            mediumText = "unspecified";
            break;
        case Networking_InterfaceMedium_Wifi:
            mediumText = "Wi-Fi";
            break;
        case Networking_InterfaceMedium_Ethernet:
            mediumText = "Ethernet";
            break;
        default:
            mediumText = "unknown-medium";
            break;
        }
        Log_Debug("INFO:   interfaceMediumType=%d (%s)\n", mediumType, mediumText);

        // Print the interface connection status
        Networking_InterfaceConnectionStatus status;
        int result = Networking_GetInterfaceConnectionStatus(interfaces[i].interfaceName, &status);
        if (result != 0) {
            Log_Debug("ERROR: Networking_GetInterfaceConnectionStatus: errno=%d (%s)\n", errno,
                strerror(errno));
            return -1;
        }
        Log_Debug("INFO:   interfaceStatus=0x%02x\n", status);
    }

    free(interfaces);

    return 0;
}

/// <summary>
///     Configure network interface, start SNTP server and TCP server.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int CheckNetworkStackStatusAndLaunchServers(void)
{
    // Check the network stack readiness and display available interfaces when it's ready.
    if (CheckNetworkStatus() != 0) {
        return -1;
    }

    // The network stack is ready, so unregister the timer event handler and launch servers.
    if (isNetworkStackReady) {
        Log_Debug("Network is ready!\n");
        dx_timerStop(&tmr_networkReady);
        //UnregisterEventHandlerFromEpoll(epollFd, timerFd);

        // Use static IP addressing to configure network interface.
        int result = ConfigureNetworkInterfaceWithStaticIp(NetworkInterface);
        if (result != 0) {
            return -1;
        }

        // Configure and start DHCP server.
        result = ConfigureAndStartDhcpSever(NetworkInterface);
        if (result != 0) {
            return -1;
        }
    }

    return 0;
}

static DX_TIMER_HANDLER(NetworkReadyPollTimerEventHandler)
{

//    Log_Debug("NetworkReadyPollTimerEventHandler() Called\n");

    // Check whether the network stack is ready.
    if (!isNetworkStackReady) {
        if (CheckNetworkStackStatusAndLaunchServers() != 0) {
            dx_terminate(ExitCode_NetworkReadyTimer_Consume);
        }
    }

}
DX_TIMER_HANDLER_END

/// <summary>
/// Power Monitor timer event:  Read the power monitor
/// </summary>
static DX_TIMER_HANDLER(powerMonitorReadData)
{

    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) != -1) {
        if (isNetworkReady) {
            pollNetBooterCurrentData();
        }
        else {
            Log_Debug("Network not ready\n");
        }
    }
    else {
        Log_Debug("Failed to get Network state\n");
    }
}
DX_TIMER_HANDLER_END

/****************************************************************************************
 * Using the passed in networkStatus value, turn on/off the connection status LEDs
 * to reflect the current connection status.
 * Red == No WiFi connection
 * Green == WiFi connected not connected to Azure
 * Blue == Connected to Azure
*****************************************************************************************/
static void setConnectionStatusLed(RGB_Status newNetworkStatus)
{
	static RGB_Status lastNetworkStatus = RGB_Invalid;

	// Nothing to see here folks, move along . . .
	if(lastNetworkStatus == newNetworkStatus){
		return;
	}

	// Update the local static variable
	lastNetworkStatus = newNetworkStatus;

	// Turn off all the LED's then set the LED corresponding to the network status
	dx_gpioOff(&red_led);
	dx_gpioOff(&green_led);
	dx_gpioOff(&blue_led);

	switch (newNetworkStatus)
	{
		case RGB_No_Network: // RED LED on
			dx_gpioOn(&red_led);
			break;
		case RGB_Network_Connected: // Green LED on
			dx_gpioOn(&green_led);
			break;
		case RGB_IoT_Hub_Connected: // Blue LED on
			dx_gpioOn(&blue_led);
			break;
		case RGB_Invalid:
		default:	
			break;		
	}
}

static DX_DEVICE_TWIN_HANDLER(dt_dev1_enable_handler, deviceTwinBinding)
{
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_BOOL) {

        dev1Enabled = *(bool*)deviceTwinBinding->propertyValue;
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
        Log_Debug("Device 1 is %s\n", (dev1Enabled ? "Enabled": "Disabled"));
        EnableDisableNetBooterDevice(1, dev1Enabled);

    } else {
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
    }
    
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_dev2_enable_handler, deviceTwinBinding)
{
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_BOOL) {

        dev2Enabled = *(bool*)deviceTwinBinding->propertyValue;
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
        Log_Debug("Device 2 is %s\n", (dev2Enabled ? "Enabled": "Disabled"));
        EnableDisableNetBooterDevice(2, dev2Enabled);


    } else {
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_telemetry_period_handler, deviceTwinBinding)
{
    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_INT) {

        int telemetryTimerTime = *(int*)deviceTwinBinding->propertyValue;
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
        Log_Debug("New telemetry period is %d seconds\n", telemetryTimerTime);
        dx_timerChange(&tmr_readPwrMonitor, &(struct timespec){telemetryTimerTime, 0});

    } else {
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);
    }
}
DX_DEVICE_TWIN_HANDLER_END

static DX_DEVICE_TWIN_HANDLER(dt_gpio_handler, deviceTwinBinding)
{
    bool gpio_level = *(bool *)deviceTwinBinding->propertyValue;

    if(deviceTwinBinding->context != NULL){
        DX_GPIO_BINDING *gpio = (DX_GPIO_BINDING*)deviceTwinBinding->context;
        
        if(gpio_level){
            dx_gpioOn(gpio);
        }
        else{
            dx_gpioOff(gpio);
        }
        dx_deviceTwinReportValue(deviceTwinBinding, deviceTwinBinding->propertyValue);

        // Update the global relay_x_enabled variables to be included in telemetry messages
        if(0 == strncmp(deviceTwinBinding->propertyName, "clickBoardRelay1", 16)){
            relay_1_enabled = (GPIO_Value_High == gpio_level ? true: false);
        }

        if(0 == strncmp(deviceTwinBinding->propertyName, "clickBoardRelay2", 16)){
            relay_2_enabled = (GPIO_Value_High == gpio_level ? true: false);
        }

        // Read the netBooter device and send up telemetry
        pollNetBooterCurrentData();
    }
}
DX_DEVICE_TWIN_HANDLER_END


/****************************************************************************************
 * Periodic timer handler to check network status and drive connction LED
*****************************************************************************************/
static DX_TIMER_HANDLER(update_network_led_handler)
{
	// Assume we don't have a network connection
    RGB_Status networkStatus = RGB_No_Network;

	// Next check for Azure connectivity staus
	if(dx_isAzureConnected()){
		networkStatus = RGB_IoT_Hub_Connected;
	}
	else if (dx_isNetworkReady()){
		networkStatus = RGB_Network_Connected;
	}
	setConnectionStatusLed(networkStatus);
}
DX_TIMER_HANDLER_END

/// <summary>
/// Handler to check for Button Presses
/// </summary>
static DX_TIMER_HANDLER(ButtonPressCheckHandler)
{
    // Assume the device comes up with the buttons at rest
    static GPIO_Value_Type buttonAState = GPIO_Value_High;
    static GPIO_Value_Type buttonBState = GPIO_Value_High;

    // Variables for the current state
    GPIO_Value_Type button_a_state;
    GPIO_Value_Type button_b_state;

    // Read both button states
    if(GPIO_GetValue(buttonA.fd, &button_a_state) < 0){
        Log_Debug("ERROR: buttonA read: errno=%d (%s)\n", errno, strerror(errno));
        dx_terminate(ExitCode_ReadButtonAError);
        return;
    }

    if(GPIO_GetValue(buttonB.fd, &button_b_state) < 0){
        Log_Debug("ERROR: buttonB read: errno=%d (%s)\n", errno, strerror(errno));
        dx_terminate(ExitCode_ReadButtonBError);
        return;
    }

    ProcessButtonState(button_a_state, &buttonAState, "buttonA");
    ProcessButtonState(button_b_state, &buttonBState, "buttonB");
}
DX_TIMER_HANDLER_END

static void ProcessButtonState(GPIO_Value_Type new_state, GPIO_Value_Type* old_state, const char* telemetry_key){

    // Did button state change?
    if(new_state != *old_state){

        // Update our local copy
        *old_state = new_state;

        // Check to see is the change is a button press
        if(new_state == GPIO_Value_Low){

            if(0 == strncmp(telemetry_key, "buttonA", 7)){
                dev1Enabled = !dev1Enabled;
                dx_deviceTwinReportValue(&dt_dev1_enabled, (void*)&dev1Enabled);
                Log_Debug("Device 1 is %s\n", (dev1Enabled ? "Enabled": "Disabled"));
                EnableDisableNetBooterDevice(1, dev1Enabled);
            }
            else if(0 == strncmp(telemetry_key, "buttonB", 7)){
                dev2Enabled = !dev2Enabled;
                dx_deviceTwinReportValue(&dt_dev2_enabled, (void*)&dev2Enabled);
                Log_Debug("Device 1 is %s\n", (dev2Enabled ? "Enabled": "Disabled"));
                EnableDisableNetBooterDevice(2, dev2Enabled);
            }
        }
        // else the button was released
    }
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
#ifdef USE_AVNET_IOTCONNECT
    dx_avnetConnect(&dx_config, NETWORK_INTERFACE);
    dx_avnetSetDebugLevel(AVT_DEBUG_LEVEL_VERBOSE);

#else     
//    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
#endif     
    
    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinUnsubscribe();
    dx_directMethodUnsubscribe();
    dx_gpioSetClose(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerEventLoopStop();
}

int main(int argc, char *argv[])
{
    dx_registerTerminationHandler();

    if (!dx_configParseCmdLineArguments(argc, argv, &dx_config)) {
        return dx_getTerminationExitCode();
    }

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