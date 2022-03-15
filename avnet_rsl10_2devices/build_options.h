/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BUILD_OPTIONS_H
#define BUILD_OPTIONS_H

// Define to build for Avnet's IoT Connect platform
//#define USE_IOT_CONNECT

// Defines how often sensor data is reported
#define TELEMETRY_SEND_PERIOD_SECONDS 30
#define TELEMETRY_SEND_PERIOD_NANO_SECONDS 0 * 1000

// Define the RSL10 data this application reports as telemetry
#define SEND_RSL10_BATTERY_DATA
#define SEND_RSL10_TEMP_HUMIDITY_DATA
//#define SEND_RSL10_MOTION_DATA

// Enable to see UART debug from PMOD
//#define ENABLE_UART_DEBUG

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