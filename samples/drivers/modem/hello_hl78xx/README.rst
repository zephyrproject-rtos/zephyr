.. zephyr:code-sample:: hello_hl78xx
   :name: Hello hl78xx modem driver

   get  & set basic hl78xx modem information & functionality with HL78XX modem APIs

Overview
********

A simple sample that can be used with only Sierra Wireless HL78XX series modems

Notes
*****

This sample uses the devicetree alias ``modem`` to identify
the modem instance to use.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/modem/hello_hl78xx
   :host-os: all
   :goals: build flash
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

   [00:00:12.840,000] <inf> hl78xx_socket: Apn="netfeasavodiot.mnc028.mcc901.gprs"
   [00:00:12.840,000] <inf> hl78xx_socket: Addr=10.149.105.74.255.255.255.252
   [00:00:12.840,000] <inf> hl78xx_socket: Gw=10.149.105.73
   [00:00:12.840,000] <inf> hl78xx_socket: DNS=141.1.1.1
   [00:00:12.840,000] <inf> hl78xx_socket: Extracted IP: 10.149.105.74
   [00:00:12.840,000] <inf> hl78xx_socket: Extracted Subnet: 255.255.255.252
   [00:00:12.840,000] <inf> hl78xx_dev: switch from run enable gprs script to carrier on
   [00:00:15.944,000] <inf> main: IP Up
   [00:00:15.944,000] <inf> main: Connected to network

   **********************************************************
   ********* Hello HL78XX Modem Sample Application **********
   **********************************************************
   [00:00:15.980,000] <inf> main: Manufacturer: Sierra Wireless
   [00:00:15.980,000] <inf> main: Firmware Version: HL7812.5.7.3.0
   [00:00:15.980,000] <inf> main: APN: netfeasavodiot
   [00:00:15.980,000] <inf> main: Imei: 351144441214500
   [00:00:15.980,000] <inf> main: RAT: NB1
   [00:00:15.980,000] <inf> main: Connection status: Not Registered
   [00:00:15.980,000] <inf> main: RSRP : -97
   **********************************************************

   [00:00:15.980,000] <inf> main: Setting new APN:
   [00:00:15.980,000] <inf> main: IP down
   [00:00:15.980,000] <inf> main: Disconnected from network
   [00:00:16.013,000] <inf> main: New APN: ""
   [00:00:16.013,000] <inf> main: Test endpoint: flake.legato.io:6000
   [00:00:17.114,000] <inf> main: Resolved: 20.29.223.5:6000
   [00:00:17.114,000] <inf> main: Sample application finished.

After startup, code performs:

#. Modem readiness check and power-on
#. Network interface setup via Zephyr's Connection Manager
#. Modem queries (manufacturer, firmware, APN, IMEI, signal strength, etc.)
#. Network registration and signal strength checks
#. Setting and verifying a new APN
#. Sending an AT command to validate communication
