.. _a01nyub-sample:

A01NYUB: Distance Sensor Sample
###############################

Description
***********

This sample application demonstrates the use of the A01NYUB distance
sensor. The driver uses uart interrupt APIs.

Requirements
************

The baudrate of the UART must be configured to 9600, and must support
interrupt driven API.

Building and Running
********************

To build the application, a board with UART interface has to be chosen,
or a custom devicetree overlay has to be provided. Here a Nucleo WL55JC
board is used. Then, connect the RX line from your Zephyr target board
to the TX line of the A01NYUB sensor. The RX line of the A01NYUB sensor
should be left floating or grounded. Then power the sensor with 3.3 to
5 V according to the datasheet.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/a01nyub
   :board: nucleo_wl55jc
   :goals: build
   :compact:

Sample Output
=============

The application will read the distance data every 2 seconds.

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.3.0-3949-g8450d42dbfb2 ***
   [00:00:00.000,000] <inf> main: Booted DFRobot A01NYUB distance sensor sample
   [00:00:00.000,000] <inf> main: sensor: distance [m]: 0.000000
   [00:00:02.000,000] <inf> main: sensor: distance [m]: 0.867000
   [00:00:04.000,000] <inf> main: sensor: distance [m]: 0.867000

   <repeats endlessly every 2 seconds>
