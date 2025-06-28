.. zephyr:code-sample:: hello_hl78xx
   :name: Hello hl78xx modem driver

   get  & set basic hl78xx modem information & functionality with HL78XX modem APIs

Overview
********

A simple sample that can be used with only Sierra Wireles HL78XX series modems

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

   **********************************************************
   ********* Hello HL78XX Modem Sample Application **********
   **********************************************************
   [00:00:16.881,000] <inf> main: Manufacturer: Sierra Wireless
   [00:00:16.881,000] <inf> main: Firmware Version: HL7812.5.5.17.0
   [00:00:16.881,000] <inf> main: APN: netfeasavod
   [00:00:16.881,000] <inf> main: Imei: 352244440111111
   [00:00:16.881,000] <inf> main: RAT: NB1
   [00:00:16.881,000] <inf> main: Connection status: Roaming Network
   [00:00:16.881,000] <inf> main: RSRP : -90
   **********************************************************

After startup, code performs:

#. Modem readiness check and power-on
#. Network interface setup via Zephyr's Connection Manager
#. Modem queries (manufacturer, firmware, APN, IMEI, signal strength, etc.)
#. Network registration and signal strength checks
#. Setting and verifying a new APN
#. Sending an AT command to validate communication
