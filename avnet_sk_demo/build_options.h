/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BUILD_OPTIONS_H
#define BUILD_OPTIONS_H

// If your application is going to connect straight to a IoT Hub or IoT Connect, then enable this define.
//#define IOT_HUB_APPLICATION

// Define if you want to build the Azure IoT Hub/IoTCentral Plug and Play application functionality
//#define USE_PNP

// Make sure we're using the IOT Hub code for the PNP configuration
#ifdef USE_PNP
#define IOT_HUB_APPLICATION
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:avnet:mt3620Starterkit;1"  // https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play 
#else
#define IOT_PLUG_AND_PLAY_MODEL_ID ""
#endif

// Define to build for Avnet's IoT Connect platform
//#define USE_IOT_CONNECT

// If this is a IoT Conect build, make sure to enable the IOT Hub application code
#ifdef USE_IOT_CONNECT
#define IOT_HUB_APPLICATION
#undef USE_PNP // Disable PNP device twins responses
#endif 

// Include SD1306 OLED code
// To use the OLED 
// Install a 128x64 OLED display onto the unpopulated J7 Display connector
// https://www.amazon.com/gp/product/B06XRCQZRX/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
// Enable the OLED_SD1306 #define below
//#define OLED_SD1306

// Include Intercore Communication code
// This will enable reading the ALST19 light sensor data from the M4 application
// To exercise the inter-core communication code load the M4 application first
//      cd ../RealTimeExampleApp/
//      azsphere device sideload deploy --image-package AvnetAlsPt19RTApp.imagepackage
// Enable the M4_INTERCORE_COMMS #define below
//#define M4_INTERCORE_COMMS

// Defines how quickly the accelerator data is read and reported
#define SENSOR_READ_PERIOD_SECONDS 5
#define SENSOR_READ_PERIOD_NANO_SECONDS 0 * 1000

// Defines how often sensor data is reported
#define TELEMETRY_SEND_PERIOD_SECONDS 30
#define TELEMETRY_SEND_PERIOD_NANO_SECONDS 0 * 1000

// Define how long after processing the haltApplication direct method before the application exits
#define HALT_APPLICATION_DELAY_TIME_SECONDS 5

// Enables I2C read/write debug
//#define ENABLE_READ_WRITE_DEBUG

// Set this flag to send telemetry data using the DevX seralizer
//#define USE_DEVX_SERIALIZATION

// Set this flag to force the device/application to send all network traffic through a 
// Proxy server.
//#define USE_WEB_PROXY
#ifdef USE_WEB_PROXY

// Define the proxy settings here . . .
#define PROXY_ADDRESS "192.168.8.2"
#define PROXY_PORT 3128
#define PROXY_USERNAME NULL
#define PROXY_PASSWORD NULL
#define NO_PROXY_ADDRESSES NULL
#endif // USE_WEB_PROXY

#endif 