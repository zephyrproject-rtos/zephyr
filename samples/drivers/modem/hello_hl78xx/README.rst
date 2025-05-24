.. zephyr:code-sample:: hello_hl78xx
   :name: Hello hl78xx modem driver

   get  & set basic hl78xx modem information & functionality with HL78XX modem APIs

Overview
********

A simple sample that can be used with only Sierra Wireles HL78XX series modems

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

   [00:00:18.368,000] <dbg> main: main: Manufecturer: Sierra Wi
   [00:00:18.368,000] <dbg> main: main: Firmware Version: HL7812.5.5.14.0
   [00:00:18.368,000] <dbg> main: main: APN: netfeasavodiot
   [00:00:18.368,000] <dbg> main: main: Imei: 351144441359727
   [00:00:18.368,000] <dbg> main: main: RAT: NB1
   [00:00:18.404,000] <dbg> main: main: RSRP : -110

After startup, code performs:

#. Modem readiness check and power-on
#. Network interface setup via Zephyr’s Connection Manager
#. Modem queries (manufacturer, firmware, APN, IMEI, signal strength, etc.)
#. Network registration and signal strength checks
#. Setting and verifying a new APN
#. Sending an AT command to validate communication
