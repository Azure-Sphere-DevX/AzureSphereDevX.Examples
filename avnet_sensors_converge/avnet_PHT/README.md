# Avnet Demo App for the PHT Click (MS 8607) Sensors Converge Sphere Workshop

This simple application interfaces with an Azure RTOS app running on one of the M4 cores.  This app simply opens a socket to the real-time app and requests sensor data.  When the sensor data is received it applies the data to one of three ranges and sets a PWM output to indicate the temperature measured.

Pressing either of the User buttons on the starter kit will "reset" the base temperature.

See the [lab document](./WorkshopLabDoc/MS8607-Lab-V1.pdf)) for details on running the demo
