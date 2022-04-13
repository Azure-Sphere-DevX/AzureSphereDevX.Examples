/*

MIT License

Copyright (c) Avnet Corporation. All rights reserved.
Author: Brian Willess

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#include "rsl10.h"

/****************************************************************************************
 * Telemetry message buffer property sets
 ****************************************************************************************/

// Number of bytes to allocate for the JSON telemetry message for IoT Hub/Central
#define JSON_BUFFER_SIZE 512

// Static buffer for telemetry data
static char telemetryBuffer[JSON_BUFFER_SIZE] = {0};

static DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "Avnet RSL10 Demo"}, 
                                                   &(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

// Forward declaration
bool isValidMsgHeader(char* messageID);

// Global variables
RSL10Device_t Rsl10DeviceList[MAX_RSL10_DEVICES];
int8_t currentRsl10DeviceIndex = -1;
int8_t numRsl10DevicesInList = 0;

// Flag to control how RSL10s are allowed to connect
// enableRSL10Onboarding = false: The Authorized field is not consulted before adding device to active list
// enableRSL10Onboarding = true:  Only Authorized RSL10 will be allowed to send temlemetry, as set by device twins
// This flag can be changed from the cloud using the "enableRSL10Onboarding" device twin
#ifdef REQUIRE_AUTHORIZATION
bool enableRSL10Onboarding = true;
#else
bool enableRSL10Onboarding = false;
#endif 

/// <summary>
///     Function to parse UART Rx messages and update global structures
/// </summary>
/// <param name="msgToParse">The message received from the UART</param>
void parseRsl10Message(char *msgToParse)
{

    char bdAddress[] = "  :  :  :  :  :  \0";
    
    #define MIN_RSL10_MSG_LENGTH 25

    // Do a sanity check to make sure that the message is large enough to be a valid message.
    // The battery message is the smallest message we expect, if this message is smaller than that,
    // then exit without doing any processing.
    if(strlen(msgToParse) < MIN_RSL10_MSG_LENGTH){
        Log_Debug("RSL10 message is not valid, message length = %d, minimum valid length is %d.\n", strlen(msgToParse), MIN_RSL10_MSG_LENGTH);
        return;
    }

    // Variable to hold the message identifier "ESD", "MSD" or "BAT"
    char messageID[3];

    // Generic message pointer for message ID and BdAddress
    RSL10MessageHeader_t *msgPtr;
    msgPtr = (RSL10MessageHeader_t*)msgToParse;

    // Index into device list for this message, set to invalid
    int8_t  Rsl10Index = -1;

    // Pull the RSL10 ID from the message
    getBdMessageID(messageID, msgPtr);

    // If the message does not have one of the expected message IDs, then discard it
    if(!isValidMsgHeader(messageID)){
        return;
    }

    // Pull the RSL10 address from the message
    getBdAddress(bdAddress, msgPtr);

    // Check to see if this devcice's MAC address has been white listed
    // if the call returns -1, then either the device is not authorized, or
    // the list is full.
    Rsl10Index = getDeviceIndex(bdAddress);

    if( Rsl10Index == -1){

        if(enableRSL10Onboarding){
            Log_Debug("Device %s is not authorized, discarding message data\n", bdAddress);
            Log_Debug("To onboard the device add it's MAC address to the insideMac or outsideMax device twin\n");

            // Send the unauthorized MAC up as telemetry so an admin can authorize the sensor

            // Allocate a buffer to send the telemetry
            char telemetryBuffer[128];

            // Define the Json string format for movement messages, the
            // actual telemetry data is inserted as the last string argument
            static const char Rsl10UnauthorizedTelemetryJson[] = "{\"unauthorizedMac\":\"%s\"}";

            snprintf(telemetryBuffer, sizeof(telemetryBuffer), Rsl10UnauthorizedTelemetryJson, bdAddress);
            // Send the telemetry message
            dx_azurePublish(telemetryBuffer, strnlen(telemetryBuffer, JSON_BUFFER_SIZE),
                                    messageProperties, NELEMS(messageProperties),
                                    &contentProperties);

        }
        return;
        
    }

    // Check to see if this device is active, if not it needs to be added to the list
    if(!Rsl10DeviceList[Rsl10Index].isActive){

        if (addRsl10DeviceToList(bdAddress, Rsl10Index)){

            Log_Debug("Add this device as index %d\n", Rsl10Index);

        } else {
            // Device could not be added!
            Log_Debug("ERROR: Could not add new device\n");
            return;
        }
    }

    // This device is authorized and already in the list at index Rsl10Index

    // Next determine which message we received and call the appropriate rouitine to pull data 
    // from the message and copy that data to this RSL10's data structure
    
    // Is this a Movement message?
    if (strcmp(messageID, "MSD") == 0) {
        rsl10ProcessMovementMessage(msgToParse, Rsl10Index);
    } 

    // Is this a Environmental message?
    else if (strcmp(messageID, "ESD") == 0) {
        rsl10ProcessEnvironmentalMessage(msgToParse, Rsl10Index);
    }

    // Is this a Battery message?
    else if (strcmp(messageID, "BAT") == 0) {
        rsl10ProcessBatteryMessage(msgToParse, Rsl10Index);
    }

    else{
        Log_Debug("Unknown message ID\n");
    }
    return;
}

// Worker routine to convert a string to an integer
int stringToInt(char *stringData, size_t stringLength)
{

    char tempString[64];
    strncpy(tempString, stringData, stringLength);
    tempString[stringLength] = '\0';
    return (int)(strtol(tempString, NULL, 16));
}

// Worker routine to convert hex data to it's string representation
void textFromHexString(char *hex, char *result, int strLength)
{
    char temp[3];

    temp[2] = '\0';
    for (int i = 0; i < strLength; i += 2) {
        strncpy(temp, &hex[i], 2);
        *result = (char)strtol(temp, NULL, 16);
        result++;
    }
    *result = '\0';
}

// Process a RSL10 Movement message
void rsl10ProcessMovementMessage(char* rxMessage, int8_t currentRsl10DeviceIndex)
{

    // Cast the pointer to reference this messages data structure
    Rsl10MotionMessage_t* msgPtr = (Rsl10MotionMessage_t*) rxMessage;

    // Call the routines to pull the data from the message.  This devices global structure is updated
    // by each routine.
    getRxRssi(&Rsl10DeviceList[currentRsl10DeviceIndex].lastRssi, &msgPtr->rssi[0]); 
    getSensorSettings(&Rsl10DeviceList[currentRsl10DeviceIndex], msgPtr);
    getAccelReadings(&Rsl10DeviceList[currentRsl10DeviceIndex], msgPtr);
    getOrientation(&Rsl10DeviceList[currentRsl10DeviceIndex], msgPtr);

    // Set the flag so we know that we have fresh data to send to IoTConnect
    Rsl10DeviceList[currentRsl10DeviceIndex].movementDataRefreshed = true;

#ifdef ENABLE_MSG_DEBUG
    Log_Debug("Rssi: %d\n", Rsl10DeviceList[currentRsl10DeviceIndex].lastRssi);
    Log_Debug("accel: %.4f, %.4f, %.4f\n", Rsl10DeviceList[currentRsl10DeviceIndex].lastAccel_raw_x, 
                                        Rsl10DeviceList[currentRsl10DeviceIndex].lastAccel_raw_y, 
                                        Rsl10DeviceList[currentRsl10DeviceIndex].lastAccel_raw_z);
    Log_Debug("Orientation: %.4f, %.4f, %.4f, %.4f\n", Rsl10DeviceList[currentRsl10DeviceIndex].lastOrientation_x, 
                                                       Rsl10DeviceList[currentRsl10DeviceIndex].lastOrientation_y,      
                                                       Rsl10DeviceList[currentRsl10DeviceIndex].lastOrientation_z,      
                                                       Rsl10DeviceList[currentRsl10DeviceIndex].lastOrientation_w);      
#endif 
}

// Process a RSL10 Environmental message
void rsl10ProcessEnvironmentalMessage(char* rxMessage, int8_t currentRsl10DeviceIndex)
{

    // Cast the pointer to reference this messages data structure
    Rsl10EnvironmentalMessage_t* msgPtr = (Rsl10EnvironmentalMessage_t*) rxMessage;

    // Call the routines to pull the data from the message.  This devices global structure is updated
    // by each routine.
    getRxRssi(&Rsl10DeviceList[currentRsl10DeviceIndex].lastRssi, msgPtr->rssi);
    getTemperature(&Rsl10DeviceList[currentRsl10DeviceIndex].lastTemperature, msgPtr);
    getHumidity(&Rsl10DeviceList[currentRsl10DeviceIndex].lastHumidity, msgPtr);
    getPressure(&Rsl10DeviceList[currentRsl10DeviceIndex].lastPressure, msgPtr);
    getAmbiantLight(&Rsl10DeviceList[currentRsl10DeviceIndex].lastAmbiantLight, msgPtr);

    // Set the flag so we know that we have fresh data to send to IoTConnect
    Rsl10DeviceList[currentRsl10DeviceIndex].environmentalDataRefreshed = true;

#ifdef ENABLE_MSG_DEBUG
            Log_Debug("RX rssi    : %d\n", Rsl10DeviceList[currentRsl10DeviceIndex].lastRssi);
            Log_Debug("Temperature: %.2f\n", Rsl10DeviceList[currentRsl10DeviceIndex].lastTemperature);
            Log_Debug("Humidity   : %.2f\n", Rsl10DeviceList[currentRsl10DeviceIndex].lastHumidity);
            Log_Debug("Pressure   : %.2f\n", Rsl10DeviceList[currentRsl10DeviceIndex].lastPressure);
#endif 

}

// Process a RSL10 Battery message
void rsl10ProcessBatteryMessage(char* rxMessage, int8_t currentRsl10DeviceIndex)
{

    // Cast the pointer to reference this messages data structure
    Rsl10BatteryMessage_t* msgPtr = (Rsl10BatteryMessage_t*) rxMessage;

    // Call the routines to pull the data from the message.  This devices global structure is updated
    // by each routine.
    getRxRssi(&Rsl10DeviceList[currentRsl10DeviceIndex].lastRssi, &msgPtr->rssi[0]);
    getBattery(&Rsl10DeviceList[currentRsl10DeviceIndex].lastBattery, msgPtr);

    // Set the flag so we know that we have fresh data to send to IoTConnect
    Rsl10DeviceList[currentRsl10DeviceIndex].batteryDataRefreshed = true;


#ifdef ENABLE_MSG_DEBUG
    Log_Debug("RX rssi    : %d\n", Rsl10DeviceList[currentRsl10DeviceIndex].lastRssi);
    Log_Debug("Battery    : %.2f V\n", Rsl10DeviceList[currentRsl10DeviceIndex].lastBattery);
#endif 
}


void getBdMessageID(char *messageID, RSL10MessageHeader_t *rxMessage){

    messageID[0] = rxMessage->msgSendRxId[0];
    messageID[1] = rxMessage->msgSendRxId[1];
    messageID[2] = rxMessage->msgSendRxId[2];
    messageID[3] = '\0';
}

// Set the global RSL10 address variable
void getBdAddress(char *bdAddress, RSL10MessageHeader_t *rxMessage)
{
    bdAddress[0] = rxMessage->BdAddress[10];
    bdAddress[1] = rxMessage->BdAddress[11];
    bdAddress[3] = rxMessage->BdAddress[8];
    bdAddress[4] = rxMessage->BdAddress[9];
    bdAddress[6] = rxMessage->BdAddress[6];
    bdAddress[7] = rxMessage->BdAddress[7];
    bdAddress[9] = rxMessage->BdAddress[4];
    bdAddress[10] = rxMessage->BdAddress[5];
    bdAddress[12] = rxMessage->BdAddress[2];
    bdAddress[13] = rxMessage->BdAddress[3];
    bdAddress[15] = rxMessage->BdAddress[0];
    bdAddress[16] = rxMessage->BdAddress[1];
}

// Set the global rssi variable from the end of the message
void getRxRssi(int16_t* rssiVariable, char *rxMessage)
{
    char tempRssi[3];
    tempRssi[0] = rxMessage[0];
    tempRssi[1] = rxMessage[1];
    tempRssi[2] = rxMessage[2];
    *rssiVariable  = (int16_t)atoi(tempRssi);
}

void getTemperature(float *temperature, Rsl10EnvironmentalMessage_t *rxMessage){

    uint16_t temp =  (uint16_t)((stringToInt(&rxMessage->temperature[2], 2) << 8) |
                                (stringToInt(&rxMessage->temperature[0], 2) << 0));
    *temperature = (float)(temp / 100.0);
    return;
}

void getHumidity(float *humidity, Rsl10EnvironmentalMessage_t *rxMessage){

    uint16_t temp =  (uint16_t)((stringToInt(&rxMessage->humidity[2], 2) << 8) |
                                (stringToInt(&rxMessage->humidity[0], 2) << 0));
    *humidity = (float)(temp / 100.0);

    return;
}
void getPressure(float *pressure, Rsl10EnvironmentalMessage_t *rxMessage){
    
    uint32_t temp =  (uint32_t)((stringToInt(&rxMessage->pressure[4], 2) << 16) |
                                (stringToInt(&rxMessage->pressure[2], 2) << 8) |
                                (stringToInt(&rxMessage->pressure[0], 2) << 0));
    *pressure = (float)(temp / 100.0);
    return;

}
void getAmbiantLight(uint16_t *ambiantLight, Rsl10EnvironmentalMessage_t *rxMessage){

    uint16_t temp =  (uint16_t)((stringToInt(&rxMessage->ambiantLight[2], 2) << 8) |
                                (stringToInt(&rxMessage->ambiantLight[0], 2) << 0));

    // Check to see if the current RLS10 device supports the ambian light sensor
    // The device will send 0xFFFF (65535) if the sensor is not supported.  In this case
    // just return 0.
    if(temp == 65535){
        *ambiantLight = 0;
    }
    else{
    *ambiantLight = temp;
    }

    return;
}

void getBattery(float *battery, Rsl10BatteryMessage_t *rxMessage){

    // Read the voltage level and covert it to Volts
    *battery =  (float)(((stringToInt(&rxMessage->battery[0], 2) << 8) |
                        (stringToInt(&rxMessage->battery[2], 2) << 0)))/1000;
}

void getSensorSettings(RSL10Device_t* currentDevPtr, Rsl10MotionMessage_t* rxMessage){

    uint8_t sensorSettings = 0;
    sensorSettings = (uint8_t)stringToInt((char*)&rxMessage->SensorSetting, 2);
    currentDevPtr->lastsampleRate = sensorSettings >> 4 & 0x0F;
    currentDevPtr->lastAccelRange = sensorSettings >> 2 & 0x03;
    currentDevPtr->lastDataType = sensorSettings & 0x03;
}

void getAccelReadings(RSL10Device_t* currentDevPtr, Rsl10MotionMessage_t* rxMessage){

    #define RAW_TO_MPS_SQUARED 32768*9.81f
    #define MPS_SQUARED_TO_G 0.102f

    int16_t rawAccel;

    // Read and calculate the x component
    rawAccel =  (int16_t)(stringToInt(&rxMessage->accel_raw_x[2], 2) << 8) | (int16_t)(stringToInt(&rxMessage->accel_raw_x[0], 2) << 0);
    currentDevPtr->lastAccel_raw_x = (float)rawAccel/RAW_TO_MPS_SQUARED*(float)(currentDevPtr->lastAccelRange*4)*MPS_SQUARED_TO_G;

    // Read and calculate the y component
    rawAccel =  (int16_t)(stringToInt(&rxMessage->accel_raw_y[2], 2) << 8) | (int16_t)(stringToInt(&rxMessage->accel_raw_y[0], 2) << 0);
    currentDevPtr->lastAccel_raw_y = (float)rawAccel/RAW_TO_MPS_SQUARED*(float)(currentDevPtr->lastAccelRange*4)*MPS_SQUARED_TO_G;

    // Read and calculate the z component
    rawAccel =  (int16_t)(stringToInt(&rxMessage->accel_raw_z[2], 2) << 8) | (int16_t)(stringToInt(&rxMessage->accel_raw_z[0], 2) << 0);
    currentDevPtr->lastAccel_raw_z = (float)rawAccel/RAW_TO_MPS_SQUARED*(float)(currentDevPtr->lastAccelRange*4)*MPS_SQUARED_TO_G;
}

void getOrientation(RSL10Device_t* currentDevPtr, Rsl10MotionMessage_t* rxMessage){

    #define ORIENTATION_DIVISOR 128.0f

    // Read and calculate the x component
    currentDevPtr->lastOrientation_x = (float)((int8_t)(stringToInt(&rxMessage->orientation_x[0], 2) << 0))/ORIENTATION_DIVISOR;

    // Read and calculate the y component
    currentDevPtr->lastOrientation_y = (float)((int8_t)(stringToInt(&rxMessage->orientation_y[0], 2) << 0))/ORIENTATION_DIVISOR;

    // Read and calculate the z component
    currentDevPtr->lastOrientation_z = (float)((int8_t)(stringToInt(&rxMessage->orientation_z[0], 2) << 0))/ORIENTATION_DIVISOR;

    // Read and calculate the z component
    currentDevPtr->lastOrientation_w = (float)((int8_t)(stringToInt(&rxMessage->orientation_w[0], 2) << 0))/ORIENTATION_DIVISOR;
}

bool addRsl10DeviceToList(char *newRsl10Address, int8_t currentIndex)
{
    // Increment the number of devies in the list and check to make sure the list is not already full, 
    // if so return false (failure)
    if (numRsl10DevicesInList++ == MAX_RSL10_DEVICES) {
        return false;
    }

    // Update the structure for this device, with the new mac address
    strncpy(Rsl10DeviceList[currentIndex].bdAddress, newRsl10Address, strlen(newRsl10Address));

    // Clear the flags that we use to know if we have fresh data to send up as telemetry
    Rsl10DeviceList[currentIndex].movementDataRefreshed = false;
    Rsl10DeviceList[currentIndex].environmentalDataRefreshed = false;
    Rsl10DeviceList[currentIndex].batteryDataRefreshed = false;
    
    // Mark the device entry as active. 
    Rsl10DeviceList[currentIndex].isActive = true;

    // If we need to add any process when we receive the first message from the device, then add it here
    Log_Debug("Add new device to list at index %d!\n", currentIndex);

    // Return the index into the array where we added the new device
    return true;
}

// Check to see if the devices MAC has been authorized
int8_t getDeviceIndex(char* deviceToCheck){

    int8_t i;
    // Check to see which mode we're in
    if(enableRSL10Onboarding){

        // Search the lsit for this MAC in the authorized field
        for(i = 0; i < MAX_RSL10_DEVICES; i++){
            if(strncmp((char*)Rsl10DeviceList[i].authorizedBdAddress, deviceToCheck, RSL10_ADDRESS_LEN) == 0){
                return i;
            }
        }
    }
    else{
        
        // Look to see if this device already has an entry
        // Find the first empty slot not in use
        for( i = 0; i < MAX_RSL10_DEVICES; i++){
            if(strncmp((char*)Rsl10DeviceList[i].bdAddress, deviceToCheck, RSL10_ADDRESS_LEN) == 0){
                return i;
            }
        }

        // We did not find the device in the list, find the first empty slot not in use
        for( i = 0; i < MAX_RSL10_DEVICES; i++){
            if(!Rsl10DeviceList[i].isActive ){
            return i;
            }
        }
    }

    // If control reached this point, then we did not find what we were looking for, return -1
    return -1;
}    

void rsl10SendTelemetry(void) {

    // Set the telemetry keys    
    strncpy(Rsl10DeviceList[0].telemetryKey, "InsideDevice", 15);
    strncpy(Rsl10DeviceList[1].telemetryKey, "OutsideDevice", 15);

    // Iterate over the device list and if active send telemetry
    for(int currentDevice = 0; currentDevice < MAX_RSL10_DEVICES; currentDevice++){

        //  If the current entry is active, then check to see if there is fresh data to send
        if(Rsl10DeviceList[currentDevice].isActive){

#ifdef SEND_RSL10_MOTION_DATA
            // Check to see if the current device has fresh motion data, if so send the telemetry
            if(Rsl10DeviceList[currentDevice].movementDataRefreshed){

                // Define the Json string format for movement messages, the
                // actual telemetry data is inserted as the last string argument
                static const char Rsl10MotionTelemetryJson[] =
                    "{\"RSL10Sensors\":{\"address\":\"%s\",\"rssi\":%d,\"acc_x\":%0.4f,\"acc_y\":%0.4f,\"acc_z\":%0.4f,\"orient_x\":%0.4f,\"orient_y\":%0.4f,\"orient_z\":%0.4f,\"orient_w\":%0.4f}}";

                snprintf(telemetryBuffer, sizeof(telemetryBuffer), Rsl10MotionTelemetryJson,
                                                                   Rsl10DeviceList[currentDevice].bdAddress,
                                                                   Rsl10DeviceList[currentDevice].lastRssi,
                                                                   Rsl10DeviceList[currentDevice].lastAccel_raw_x,
                                                                   Rsl10DeviceList[currentDevice].lastAccel_raw_y,
                                                                   Rsl10DeviceList[currentDevice].lastAccel_raw_z,
                                                                   Rsl10DeviceList[currentDevice].lastOrientation_x,
                                                                   Rsl10DeviceList[currentDevice].lastOrientation_y,
                                                                   Rsl10DeviceList[currentDevice].lastOrientation_z,
                                                                   Rsl10DeviceList[currentDevice].lastOrientation_w);

                Log_Debug("Send telemetry: %s\n", telemetryBuffer);

#ifdef USE_IOT_CONNECT

                // Send the telemetry message
                dx_avnetPublish(telemetryBuffer, strlen(telemetryBuffer), messageProperties, NELEMS(messageProperties), &contentProperties,NULL);

#else // !USE_IOT_CONNECT

                // Send the telemetry message
                dx_azurePublish(telemetryBuffer, strnlen(telemetryBuffer, JSON_BUFFER_SIZE),
                                    messageProperties, NELEMS(messageProperties),
                                    &contentProperties);                
#endif                 
                // Clear the flag so we don't send this data again
                Rsl10DeviceList[currentDevice].movementDataRefreshed = false;

            }
#endif // SEND_RSL10_MOTION_DATA 
#ifdef SEND_RSL10_TEMP_HUMIDITY_DATA
            // Check to see if the current device has fresh environmental data, if so send the telemetry
            if (Rsl10DeviceList[currentDevice].environmentalDataRefreshed){

                // actual telemetry data is inserted as the last string argument
                static const char Rsl10EnvironmentalTelemetryJson[] =
                    "{\"address\":\"%s\",\"rssi%s\":%d,\"temp%s\":%0.2f,\"humidity%s\": %0.2f,\"pressure%s\": %0.2f}";

                snprintf(telemetryBuffer, sizeof(telemetryBuffer), Rsl10EnvironmentalTelemetryJson,
                                                                   Rsl10DeviceList[currentDevice].bdAddress,
                                                                   Rsl10DeviceList[currentDevice].telemetryKey,
                                                                   Rsl10DeviceList[currentDevice].lastRssi,
                                                                   Rsl10DeviceList[currentDevice].telemetryKey,
                                                                   Rsl10DeviceList[currentDevice].lastTemperature,
                                                                   Rsl10DeviceList[currentDevice].telemetryKey,
                                                                   Rsl10DeviceList[currentDevice].lastHumidity,
                                                                   Rsl10DeviceList[currentDevice].telemetryKey,
                                                                   Rsl10DeviceList[currentDevice].lastPressure);
                Log_Debug("Send telemetry: %s\n", telemetryBuffer);

#ifdef USE_IOT_CONNECT

                // Send the telemetry message
                dx_avnetPublish(telemetryBuffer, strlen(telemetryBuffer), messageProperties, NELEMS(messageProperties), &contentProperties,NULL);

#else // !USE_IOT_CONNECT

                // Send the telemetry message
                dx_azurePublish(telemetryBuffer, strnlen(telemetryBuffer, JSON_BUFFER_SIZE),
                                    messageProperties, NELEMS(messageProperties),
                                    &contentProperties);                
#endif                 
    
                // Clear the flag so we don't send this data again
                Rsl10DeviceList[currentDevice].movementDataRefreshed = false;


                // Clear the flag so we don't send this data again
                Rsl10DeviceList[currentDevice].environmentalDataRefreshed = false;
            }
#endif // SEND_RSL10_TEMP_HUMIDITY_DATA
#ifdef SEND_RSL10_BATTERY_DATA
            // Check to see if the current device has fresh battery data, if so send the telemetry
            if (Rsl10DeviceList[currentDevice].batteryDataRefreshed){

                // Define the Json string format for battery messages, the
                // actual telemetry data is inserted as the last string argument
                static const char Rsl10BatteryTelemetryJson[] = "{\"address\":\"%s\",\"rssi%s\":%d,\"bat%s\":%0.2f}";

                snprintf(telemetryBuffer, sizeof(telemetryBuffer), Rsl10BatteryTelemetryJson,
                                                                   Rsl10DeviceList[currentDevice].bdAddress,
                                                                   Rsl10DeviceList[currentDevice].telemetryKey,
                                                                   Rsl10DeviceList[currentDevice].lastRssi,
                                                                   Rsl10DeviceList[currentDevice].telemetryKey,
                                                                   Rsl10DeviceList[currentDevice].lastBattery);
                Log_Debug("Send telemetry: %s\n", telemetryBuffer);

#ifdef USE_IOT_CONNECT

                // Send the telemetry message
                dx_avnetPublish(telemetryBuffer, strlen(telemetryBuffer), messageProperties, NELEMS(messageProperties), &contentProperties,NULL);

#else // !USE_IOT_CONNECT

                // Send the telemetry message
                dx_azurePublish(telemetryBuffer, strnlen(telemetryBuffer, JSON_BUFFER_SIZE),
                                    messageProperties, NELEMS(messageProperties),
                                    &contentProperties);                
#endif                 

                // Clear the flag so we don't send this data again
                Rsl10DeviceList[currentDevice].batteryDataRefreshed = false;

            }
#endif // SEND_RSL10_BATTERY_DATA
            
        }
    }
}

bool isValidMsgHeader(char* messageID){

    if(strncmp(messageID, "MSD", 3)== 0){
        return true;
    }
    else if(strncmp(messageID, "BAT", 3) == 0){
        return true;
    }
    else if(strncmp(messageID, "ESD", 3) == 0){
        return true;
    }
    return false;
}