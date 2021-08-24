.. _ublox_sara_n310:

SARA-N310 modem driver sample
########################################################

Overview
********

This is an application that tests basic socket functionality of the N310 modem. It allows the application to use basic socket functions (connect, sendto, recvfrom, close). Additionally it shows the use of PSM settings of the N310 modem. These can be configured application-side.

It also showcases retrieving modem information and rebooting the module using the modem pins. V_INT is used to check whether the module is asleep or not.

Building and Running
********************

Build the application for the stm32l1_disco board, and connect the UART pins of the SARA-N310 to the UART_2 pins (PA2 and PA3) of the STM32L1. The PWR_ON and V_INT pins of the SARA-N310 should be connected to PC1 and PC3 pins of the STM32L1 respectively. The IP used in the address struct in the sample application is the IP of the application or server you wish to send messages to.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/ublox_sara_n310
   :board: stm32l1_disco
   :goals: build
   :compact:

Sample output
=============

The results are printed and can be seen over UART_1 by enabling logging (CONFIG_LOG) in prj.conf and should look as follows:

.. code-block:: console

   <inf> modem_ublox_sara_n310: Starting modem...
   <inf> modem_ublox_sara_n310: Initializing modem pins.
   <inf> modem_ublox_sara_n310: Done.
   <inf> modem_ublox_sara_n310: Modem is ready.
   <inf> main: Manufacturer: u-blox
   <inf> main: Model: SARA-N310
   <inf> main: Revision: <revision>
   <inf> main: ICCID: <iccid>
   <inf> main: IMEI: <imei>
   <inf> main: IP: <IP>
   <inf> main: Network state: 5
   <inf> modem_ublox_sara_n310: CSCLK set with current configuration: 2
   <inf> modem_ublox_sara_n310: CPSMS set with current configuration: 1, "01011111", "00000000"
   <inf> modem_ublox_sara_n310: NVSETPM set with current configuration: 10
   <inf> main: Connecting...
   <inf> modem_ublox_sara_n310: Socket 1 created.
   <inf> main: Sending...
   <inf> modem_ublox_sara_n310: +UUSOR[D|F] received
   <inf> modem_ublox_sara_n310: Sleep mode URC: 1
   <inf> main: Receiving...
   <inf> main: Received: <data as it was sent>
   <inf> main: Sending...
   <inf> modem_ublox_sara_n310: Sleep mode URC: 0
   <inf> modem_ublox_sara_n310: ++USOR[D|F] received
   <inf> modem_ublox_sara_n310: Sleep mode URC: 1
   etc.
   ...
