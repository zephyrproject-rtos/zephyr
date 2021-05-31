.. _ublox_sara_n310:

SARA-N310 modem driver sample
########################################################

Overview
********

This is an application that tests basic socket functionality of the N310 modem. The following network functions are tested in the application loop:
- socket()
- connect()
- sendto()
- recvfrom()
- close()

It also showcases retrieving modem information and rebooting the module.

Building and Running
********************

Build the application for the stm32l1_disco board, and connect the UART pins of the SARA-N310 to the UART_2 pins (PA2 and PA3) of the STM32L1. The IP used in the address struct in the sample application is the IP of the application or server you wish to send messages to.

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
   <inf> modem_ublox_sara_n310: Modem is ready.
   <inf> main: Manufacturer: u-blox
   <inf> main: Model: SARA-N310
   <inf> main: Revision: <revision>
   <inf> main: ICCID: <iccid>
   <inf> main: IMEI: <imei>
   <inf> main: IP: <IP>
   <inf> main: Network state: 5
   <inf> modem_ublox_sara_n310: Socket 1 created.
   <inf> modem_ublox_sara_n310: +UUSOR[D|F] received
   <inf> main: Received: <data as it was sent>
   <inf> modem_ublox_sara_n310: Socket 1 closed.
   ...
