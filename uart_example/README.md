# UART usage

[UART usage documentation](https://github.com/Azure-Sphere-DevX/AzureSphereDevX.Examples/wiki/Working-with-UARTS)

This example uses a UART loopback connection to exercise the dx_uart implementation.  To use the example insert a jumper wire between the Avnet Starter Kit Click Socket #1 TX and RX signals.

Use the Starter Kit buttons to send unique messages out the UART.  The UART handler will catch the UART data and output the message to debug.