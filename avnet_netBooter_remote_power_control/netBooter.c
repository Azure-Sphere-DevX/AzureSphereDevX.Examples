/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

   // This sample C application for Azure Sphere periodically downloads and outputs the index web page
   // at example.com, by using cURL over a secure HTTPS connection.
   // It uses the cURL 'easy' API which is a synchronous (blocking) API.
   //
   // It uses the following Azure Sphere libraries:
   // - log (messages shown in Visual Studio's Device Output window during debugging);
   // - storage (device storage interaction);
   // - curl (URL transfer library).

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <curl/curl.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/storage.h>

#include "netBooter.h"
#include "dx_avnet_iot_connect.h"
//#include "main.h"

// Global variables to store and share telemetry/device twin data
float dev1Current;
float dev2Current;
float dev1CurrentThreshold;
float dev2CurrentThreshold;
bool relay_1_enabled;
bool relay_2_enabled;

/****************************************************************************************
 * Telemetry Details
 ****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub
#define JSON_MESSAGE_BYTES 256
char msgBuffer[JSON_MESSAGE_BYTES] = {0};

// Define telemetry message format
const char netBooterTelemetry[] = "{\"port_1_enabled\": %s, \"port_1_current\":\"%.2f\", \"port_2_enabled\":%s, \"port_2_current\":\"%.2f\", \"relay_1_enabled\": %s, \"relay_2_enabled\":%s}";

DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "netBoot"}, &(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};


bool isFreshData;
extern char deviceIpAddress[];

void SendBooleanTelemetry(const unsigned char* key, const bool value);

/// <summary>
///     Data pointer and size of a block of memory allocated on the heap.
/// </summary>
typedef struct {
    char* data;
    size_t size;
} MemoryBlock;

/// <summary>
///     Callback for curl_easy_perform() that copies all the downloaded chunks in a single memory
///     block.
/// <param name="chunks">The pointer to the chunks array</param>
/// <param name="chunkSize">The size of each chunk</param>
/// <param name="chunksCount">The count of the chunks</param>
/// <param name="memoryBlock">The pointer where all the downloaded chunks are aggregated</param>
/// </summary>
static size_t StoreDownloadedDataCallback(void* chunks, size_t chunkSize, size_t chunksCount,
    void* memoryBlock)
{
    MemoryBlock* block = (MemoryBlock*)memoryBlock;

    size_t additionalDataSize = chunkSize * chunksCount;
    block->data = realloc(block->data, block->size + additionalDataSize + 1);
    if (block->data == NULL) {
        Log_Debug("Out of memory, realloc returned NULL: errno=%d (%s)'n", errno, strerror(errno));
        abort();
    }

    memcpy(block->data + block->size, chunks, additionalDataSize);
    block->size += additionalDataSize;
    block->data[block->size] = 0; // Ensure the block of memory is null terminated.

    return additionalDataSize;
}

/// <summary>
///     Logs a cURL error.
/// </summary>
/// <param name="message">The message to print</param>
/// <param name="curlErrCode">The cURL error code to describe</param>
static void LogCurlError(const char* message, int curlErrCode)
{
    Log_Debug(message);
    Log_Debug(" (curl err=%d, '%s')\n", curlErrCode, curl_easy_strerror(curlErrCode));
}


/// <summary>
///   Enable/Disable netBoot devices over HTTP protocol using cURL./// </summary>
bool EnableDisableNetBooterDevice(int devNum, bool newState)
{

    CURL* curlHandle = NULL;
    CURLcode res = 0;
    MemoryBlock block = { .data = NULL, .size = 0 };

    bool returnVal = false;

    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) < 0) || !isNetworkingReady) {
        Log_Debug("\nNot doing download because there is no internet connectivity.\n");
        goto exitLabel;
    }

    //    Log_Debug("Send curl message\n");

    // Init the cURL library.
    if ((res = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK) {
        LogCurlError("curl_global_init", res);
        goto exitLabel;
    }

    if ((curlHandle = curl_easy_init()) == NULL) {
        Log_Debug("curl_easy_init() failed\n");
        goto cleanupLabel;
    }

    // Construct the Url + command
    static char url[64] = { 0 };
    static const char* URLMsgTemplate = "http://%s/cmd.cgi?$A3%%20%d%%20%d";
    int len = snprintf(url, sizeof(url), URLMsgTemplate, deviceIpAddress, devNum, newState);
    if (len < 0) {
        Log_Debug("call to snprintf failed!\n");
        goto cleanupLabel;
    }

    // Set the URL using the dynamic URL created above
    if (curl_easy_setopt(curlHandle, CURLOPT_URL, url) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_URL", res);
        goto cleanupLabel;
    }

    // Make it a post
    if (curl_easy_setopt(curlHandle, CURLOPT_POST, 1) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_POST", res);
        goto cleanupLabel;
    }

    // Supply the default username
    if (curl_easy_setopt(curlHandle, CURLOPT_USERNAME, "admin") != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_USERNAME", res);
        goto cleanupLabel;
    }

    // Supply the default password
    if (curl_easy_setopt(curlHandle, CURLOPT_PASSWORD, "admin") != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_PASSWORD", res);
        goto cleanupLabel;
    }

    // Set up callback for cURL to use when downloading data.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, StoreDownloadedDataCallback)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_FOLLOWLOCATION", res);
        goto cleanupLabel;
    }

    // Set the custom parameter of the callback to the memory block.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void*)&block)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_WRITEDATA", res);
        goto cleanupLabel;
    }

    // Perform the download of the web page.
    if ((res = curl_easy_perform(curlHandle)) != CURLE_OK) {
        LogCurlError("curl_easy_perform", res);
    }
    else {

                // Set the delimiter for the strtok routine
        char delim[] = ",";

        // block.data contains the comma delimited response.  Pull each pice of 
        // data in the order it's supplied using he strtok function
        // 1: Response Code

        // Pull the response Code from the data
        char* ptr = strtok(block.data, delim);

        // Verify the strtok returned data
        if (ptr == NULL) {
            Log_Debug("Call to strtok(blick.data) returned NULL\n");
            goto cleanupLabel;
        }

        // Check to make sure we have a valid response
        if (strncmp(ptr, "$A0", 3) != 0) {
            Log_Debug("Invalid response from NetBoot device\n");
        }
        else {
            returnVal = true;
        }

    }

cleanupLabel:
    // Clean up allocated memory.
    free(block.data);

    // Clean up sample's cURL resources.
    curl_easy_cleanup(curlHandle);

    // Clean up cURL library's resources.
    curl_global_cleanup();

exitLabel:
    
    // We just changed the state of one of the outlets, send up current status.
    pollNetBooterCurrentData();
    
    return returnVal;
}

/// <summary>
///   Pull netBoot data over HTTP protocol using cURL.
/// </summary>
void pollNetBooterCurrentData(void)
{

#define ENABLE_NETBOOTER_DEBUG 

    CURL* curlHandle = NULL;
    CURLcode res = 0;
    MemoryBlock block = { .data = NULL, .size = 0 };

    // Variables to keep track of when the current exceeds the on/off threshold 
    static bool dev1On = false;
//    static bool dev1OnLast = false;
    static bool dev2On = false;
//    static bool dev2OnLast = false;

    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) < 0) || !isNetworkingReady) {
        Log_Debug("\nNot doing download because there is no internet connectivity.\n");
        goto exitLabel;
    }

    // Init the cURL library.
    if ((res = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK) {
        LogCurlError("curl_global_init", res);
        goto exitLabel;
    }

    if ((curlHandle = curl_easy_init()) == NULL) {
        Log_Debug("curl_easy_init() failed\n");
        goto cleanupLabel;
    }

    // Dynamically construct the URL
    char curlURL[36] = { 0 };
    static const char* curlURLTemplate = "http://%s/cmd.cgi?$A5";

    int len = snprintf(curlURL, sizeof(curlURL), curlURLTemplate, deviceIpAddress);
    if (len < 0) {
        goto cleanupLabel;
    }

    // Set the URL, we also include the device status command "$A5"
    if (curl_easy_setopt(curlHandle, CURLOPT_URL, curlURL) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_URL", res);
        goto cleanupLabel;
    }

    // Make it a post
    if (curl_easy_setopt(curlHandle, CURLOPT_POST, 1) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_POST", res);
        goto cleanupLabel;
    }

    // Set output level to verbose.
//    if ((res = curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1L)) != CURLE_OK) {
//        LogCurlError("curl_easy_setopt CURLOPT_VERBOSE", res);
//        goto cleanupLabel;
//    }

    // Let cURL follow any HTTP 3xx redirects.
    // Important: any redirection to different domain names requires that domain name to be added to
    // app_manifest.json.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_FOLLOWLOCATION", res);
        goto cleanupLabel;
    }

    // Supply the default username
    if (curl_easy_setopt(curlHandle, CURLOPT_USERNAME, "admin") != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_USERNAME", res);
        goto cleanupLabel;
    }

    // Supply the default password
    if (curl_easy_setopt(curlHandle, CURLOPT_PASSWORD, "admin") != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_PASSWORD", res);
        goto cleanupLabel;
    }

    // Set up callback for cURL to use when downloading data.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, StoreDownloadedDataCallback)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_FOLLOWLOCATION", res);
        goto cleanupLabel;
    }

    // Set the custom parameter of the callback to the memory block.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void*)&block)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_WRITEDATA", res);
        goto cleanupLabel;
    }

    // Set the timeout
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT, 1L)) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_TIMEOUT", res);
        goto cleanupLabel;
    }

    // Specify a user agent.
    if ((res = curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0")) != CURLE_OK) {
        LogCurlError("curl_easy_setopt CURLOPT_USERAGENT", res);
        goto cleanupLabel;
    }

    // Perform the download of the web page.
    if ((res = curl_easy_perform(curlHandle)) != CURLE_OK) {
        LogCurlError("curl_easy_perform", res);
    }
    else {

#ifdef  ENABLE_NETBOOTER_DEBUG
        Log_Debug("%s\n", block.data);
#endif 

        // The strtok() routine will modify the data buffer, since we don't own
        // this memory we'll make a local copy before processing the response.
        char* messageBuffer = malloc(block.size);
        memcpy(messageBuffer, block.data, block.size);
        if (messageBuffer == NULL) {
            Log_Debug("memory allocation failed\n");
            goto mallocErrorLabel;
        }
        
    // Set the delimiter for the strtok routine
        char delim[] = ",";

        // block.data contains the comma delimited response.  Pull each pice of 
        // data in the order it's supplied using he strtok() function
        // 1: Response Code
        // 2: Outlet State 
        // 3: Current device #2
        // 4: Current device #1

        // Pull the response Code from the data
        char* ptr = strtok(messageBuffer, delim);

        // Verify the strtok returned data
        if (ptr == NULL) {
            Log_Debug("Call to strtok(blick.data) returned NULL\n");
            goto mallocErrorLabel;
        }

#ifdef ENABLE_NETBOOTER_DEBUG
        Log_Debug("Response: %s\n", ptr);
#endif 

        // Check to make sure we have a valid response
        if (strcmp(ptr, "$A0") == 0) {

            // Pull the On/Off state from the data
            ptr = strtok(NULL, delim);
            int outletState = atoi(ptr);
#ifdef ENABLE_NETBOOTER_DEBUG
            // Parse out the device status enabled/disabled
            Log_Debug("Device #2 is ");
            if (outletState & DEVICE_TWO_MASK) {
                Log_Debug("Enabled\n");
                dev2On = true;
            }
            else {
                Log_Debug("Disabled\n");
                dev2On = false;
            }
            
            // Parse out the device status enabled/disabled
            Log_Debug("Device #1 is ");
            if (outletState & DEVICE_ONE_MASK) {
                Log_Debug("Enabled\n");
                dev1On = true;
            }
            else {
                Log_Debug("Disabled\n");
                dev1On = false;
            }
#endif 
            // Pull outlet #2 Current
            ptr = strtok(NULL, delim);
            dev2Current = (float)atof(ptr);
#ifdef ENABLE_NETBOOTER_DEBUG
            Log_Debug("Outlet #2 Current: %.02f\n", dev2Current);
#endif 
            // Pull outlet #1 Current
            ptr = strtok(NULL, delim);
            dev1Current = (float)atof(ptr);

            // Set the flag stating that we read fresh data
//            isFreshData = true;
#ifdef ENABLE_NETBOOTER_DEBUG
            Log_Debug("Outlet #1 Current: %.02f\n", dev1Current);
#endif 
            if(dx_isAvnetConnected()){
                // construct and send the telemetry message
                snprintf(msgBuffer, JSON_MESSAGE_BYTES, netBooterTelemetry, (dev1On ? "true" : "false"), dev1Current, (dev2On ? "true" : "false"), dev2Current, (relay_1_enabled ? "true" : "false"), (relay_2_enabled ? "true" : "false"));
                dx_avnetPublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties, NULL);
            }
        }
        else {
            Log_Debug("Invalid response from NetBoot device\n");
        }

mallocErrorLabel:
        free(messageBuffer);
    }
 
cleanupLabel:

    // Clean up allocated memory.
    free(block.data);
    
    // Clean up sample's cURL resources.
    curl_easy_cleanup(curlHandle);

    // Clean up cURL library's resources.
    curl_global_cleanup();

exitLabel:

    return;
}
