# Generic RTApp Interface

This example works in conjunction with the Azure RTOS applications in the GitHub repo [here](https://github.com/Avnet/avnet-azure-sphere-AzureRTOS).  I wanted to develop a collection of real-time applications that could be pre-built and then either side loaded or deployed to an Azure Sphere Device OTA.  This application looks for up to two real-time applications running on the device.  It assumes that the Component IDs are either "f6768b9a-e086-4f5a-8219-5ffe9684b001" for real-time app #1, or  "f6768b9a-e086-4f5a-8219-5ffe9684b002" for real-time app #2.  

The high level application sends a generic command to the real-time applications to read their sensors and return JSON telemetry strings containing the sensor data.  The telemetry strings are received at the high level application and forwarded to the IoTHub.  See the README.md file in the Azure RTOS repo for all the details.  The real-time applications can be swapped out to utilize different sensors.

This application assumes that it will connect to an IoTHub.  The app_manifest.json file needs to be updated with the IoTHub hostname, the DPS Scope ID, and the Azure Sphere Tenant GUID.

The application implements three Device Twins . . .

* "telemetryTimerAllApps"
    * Requests each real time application to read their sensors and return telemetry JSON at the interval requested 
    * Takes an integer >= 0
    * 0 == Don't request any telemetry
    * integer > 0 requests telemetry every X seconds
    * The timer default is to request telemetry data ever 15 seconds
* "rtApp1AutoTelemetryTimer"
    * Requests real time application #1 to automatically read its sensors and return telemetry JSON at the interval requested 
    * Takes an integer >= 0
    * 0 == Stop the auto telemetry function
    * Integer > 0 requests telemetry every X seconds
    * The default is 0, off
* "rtApp2AutoTelemetryTimer"
    * Requests real time application #2 to automatically read its sensors and return telemetry JSON at the interval requested 
    * Takes an integer >= 0
    * 0 == Stop the auto telemetry function
    * Integer > 0 requests telemetry every X seconds
    * The default is 0, off
