# Sample: AvnetRSL10Sensor

This sample implements a application that can monitor and report sensor data for 2 RSL10 BLE sensors devices. 

This sample uses a BLE device connected to an Azure Sphere device to capture, parse and transmit RSL10 Sensor data to Azure.  The Avnet BLE PMOD runs a modified Laird SmartBasic application (Repeater Gateway) that listens for broadcast messages from RSL10 devices, then transmits the message over a UART connection to the Azure Sphere Starter Kit.  The Azure Sphere application parses the message, verifies that the RSL10 is authorized to send data from the application and sends up any telemetry/events as telemetry to Azure.

The application will only send telemetry if it's recieved updated data from one of the authorized RSL10 devices, and it will only send data for the device that it received data from.  The default telemetry send period is 30 seconds.  That is as long as new data is received within the 30-second window, telemetry will be sent every 30-seconds.  The telemetry period can be modified by updateing the telemetryPollPeriod device twin.

Since the RSL10 devices do not have a label with their MAC address, this application implements a device twin called "enableRSL10Onboarding."  When enableRSL10Onboarding is set to true and the appliction receives an RSL10 message, it will check to see if the message is from an authorized RSL10, and if it's NOT authorized, the application will send a telemetry message containing the MAC address.  This way an admin can monitor telemetry messages to identify RSL10 MAC address' that should be added to one the authorized MAC address device twins, either "insideMac" or "outsideMac."

## Required hardware

* 1 - [Avnet Azure Sphere Starter Kit](http://avnet.me/mt3620-kit)
* 1 - [Avnet BLE PMOD](https://www.avnet.com/shop/us/products/avnet-engineering-services/aes-pmod-nrf-ble-g-3074457345642996769)
* 2 - [ON RSL10s](https://www.avnet.com/shop/us/products/on-semiconductor/rsl10-sense-gevk-3074457345639458682/)

## Hardware Configuration

### Update the BLE PMOD fw/sw

The BLE PMOD needs to be updated with the latest SmartBasic firmware and then loaded with the SmartBasic application.

* Update the PMOD's onboard BL654 module to FW Version 29.4.6.0 or newer
   * [Link to firmware, scroll way down to documentation section](https://www.lairdconnect.com/wireless-modules/bluetooth-modules/bluetooth-5-modules/bl654-series-bluetooth-module-nfc)
* Load the $autorun$.BT510.gateway.sb SmartBasic application onto the BL654 module
   * This custom SmartBasic application is located in this repo under the RSL10Repeater folder 
   * Build and load the $autorun$.BT510.gateway.sb application using the Laird UwTerminalX utility
   * If you get compile errors, make sure you updated the firmware on your device to 29.4.6.0 or greater and your restarted the device after the update!
  
### Connect the BLE PMOD to the StarterKit

* Solder a 2x6 Right angle header onto the Starter Kit
* Plug the updated BLE PMOD into the new 2x6 header
* Update the azsphere_board.txt file with your target hardware platform
   * avnet_mt3620_sk OR
   * avnet_mt3620_sk_rev2

[Starter Kit](./media/ConnectorTop.jpg)

### Update the firmware on the RSL10

* Install the [SEGGER J-Link Software and Debugger tools](https://www.segger.com/downloads/jlink/)
* Connect your RSL10 to a J-Link debugger
  * I use [this cable](https://www.segger.com/products/debug-probes/j-link/accessories/adapters/6-pin-needle-adapter/), and some creative connections between the two devices
* Open the SEGGER J-Link Lite utility
  * Select the RSL10 Device
  * Keep the interface set to SWD and Speed 4000 kHz
  * Open the /RSL10Firmware/sense_example_broadcaster_V1.0.1.hex file
  * Click on the Program Device button

## Build Options

The application behavior can be defined in the build_options.h file

* USE_IOT_CONNECT enables support to connect to Avnet IoTConnect Cloud solution platform
* SEND_RSL10_BATTERY_DATA enables sending battery readings as telemetry
* SEND_RSL10_TEMP_HUMIDITY_DATA enables sending environmental data as telemetry
* SEND_RSL10_MOTION_DATA enables sending motion data as telemeyry

## Runtime configuration

The application is configured from the cloud

### Authorize RSL10s to connect to the appliation

The application implements two device twins for authroizing RSL10 devices, "insideMac" and "outsideMac."  The configuration steps are . . . 

* 1 - Power up the Azure Sphere application
* 2 - Verify that the application connects to the IoTHub
* 3 - Using the Azure Portal or the Azure IoTExplorer application verify that the device twin "enableRSL10Onboarding" is set to true
* 4 - Power up the first RSL10 device
* 5 - Using the Azure IoTExplorer monitor telemetry from the device
* 6 - Copy the RSL10 Mac address from the telemetry message
* 7 - Update the "insideMac" device twin field with the Mac address (install this RSL10 device inside the building)
* 8 - Power up the second RSL10 device
* 9 - Using the Azure IoTExplorer monitor telemetry from the device
* 10 - Copy the RSL10 Mac address from the telemetry message
* 11 - Update the "outsideMac" device twin field with the Mac address (install this RSL10 device outside the building)
* 12 - Update the "enableRSL10Onboarding" device twin to false

### Avnet's IoTConnect configuration

If you're using Avnet's IoTConnect cloud solution you can use the device template JSON file located in the IoTConnect folder to define all the device to Cloud (D2C) messages and device twins.

### Configure the application to connect to your Azure Solution

Use the instructions below to configure your app_manifest.json file to allow a connection to an Azure IoT Hub.

**IMPORTANT**: This sample application requires customization before it will compile and run. Follow the instructions in this README and in IoTCentral.md and/or IoTHub.md to perform the necessary steps.

This application does the following:

Before you can run the sample, you must configure either an Azure IoT Central application or an Azure IoT hub, and modify the sample's application manifest to enable it to connect to the Azure IoT resources that you configured.

## Prerequisites

The sample requires the following software:

- Azure Sphere SDK version 21.01 or higher. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk), if necessary.
- An Azure subscription. If your organization does not already have one, you can set up a [free trial subscription](https://azure.microsoft.com/free/?v=17.15).

## Preparation

**Note:** By default, this sample targets the [Avnet Azure Sphere Starter Kit](http://avnet.me/mt3620-kit) hardware.  To build the sample for different Azure Sphere hardware, change the Target Hardware Definition Directory in the project properties. For detailed instructions, see the [README file in the HardwareDefinitions folder](../../HardwareDefinitions/README.md).

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/overview).
2. Clone the Azure Sphere Samples repository on GitHub and navigate to the AvnetRSL10Sensor folder.
3. Connect your Azure Sphere device to your computer by USB.
4. Enable a network interface on your Azure Sphere device and verify that it is connected to the internet.
5. Open an Azure Sphere Developer Command Prompt and enable application development on your device if you have not already done so:

   **azsphere device enable-development**

## Run the sample

- [Run the sample with Azure IoT Central](./IoTCentral.md)
- [Run the sample with an Azure IoT Hub](./IoTHub.md)
