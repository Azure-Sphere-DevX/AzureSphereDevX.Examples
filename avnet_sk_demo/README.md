# Azure IoT Telemetry, Hub Direct Methods and Device Twins

This is an end to end sample pulling together publishing telemetry, device twins, direct methods, timer, gpio and more.

## Select your build options

1. Open build_optons.h

### Non-connected build
The non-connected build with read the Avnet Starter Kit on-board sensors and the sensor readings to debug

1. Open CMakeLists.txt and select your hardware revision from lines 12-13
1. Connect your kit to your development computer 
1. Press F5 to build/load/run the application

### Connected build option

1. Open CMakeLists.txt and select your hardware revision from lines #12-#13
1. Open build_options.h
1. Enable the '''#define IOT_HUB_APPLICATION'' ~line #6
1. Configure your [Azure Resources](./IoTHub.md) (IoTHub, DPS), or configure an [IoTCentral Application](./IoTCentral.md)
    1. If you are connecting to IoTCentral you must also enable the USE_PNP flag on ~line #9
1. Open app_manifest.json
    1. Set ID Scope
    1. Set Allowed connections
    1. Set DeviceAuthentication
1. Connect your kit to your development computer 
1. Press F5 to build/load/run the application

## Other build options
Please review the build_options.h file for all the different build options

For more information refer to:

1. [Adding the Azure Sphere DevX library](https://github.com/gloveboxes/AzureSphereDevX/wiki/Adding-the-DevX-Library)
1. [Azure Messaging](https://github.com/gloveboxes/AzureSphereDevX/wiki/IoT-Hub-Sending-messages)
1. [Device Twins](https://github.com/gloveboxes/AzureSphereDevX/wiki/IoT-Hub-Device-Twins)
1. [Direct Methods](https://github.com/gloveboxes/AzureSphereDevX/wiki/IoT-Hub-Direct-Methods)
1. [GPIO](https://github.com/gloveboxes/AzureSphereDevX/wiki/Working-with-GPIO)
