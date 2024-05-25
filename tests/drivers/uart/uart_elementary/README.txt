The purpose of this test is to validate basic UART driver functions,
that are not tested elsewhere.
UART interrupt mode is used for the tests purpose.

Hardware setup required for these tests:
For single uart conviguration - UART0 TX connected to RX and CTS to RTS
For dual uart configuratiom - UART0 and UART1 TXs and RXs cross-connected

These test cases cover:
- UART configuration,
- UART error check,
- UART callback setup,
- Dual UART transmission with matched and mismatched configurations
