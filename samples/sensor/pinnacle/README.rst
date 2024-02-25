.. _pinnacle:

Cirque Pinnacle 1CA027: GlidePoint SPI trackpad sensor
######################################################

Description
***********

This sample application reads the fingerpoint position, X and Y coordinates,
on the trackpad alongside with the measure of the capacitive field, Z-level,
in the triggering mode.

The sensor is configured to send samples with coordinates of a finger every
10 ms when a touch is detected within active range and idle samples, with all
coordinates set to 0, when the touch disappears. The sensor sends up to 20
idle samples until a new touch is detected.

References
**********

 - 1CA027: https://www.cirque.com/gen2gen3-asic-details
 - GlidePoint: https://www.cirque.com/glidepoint-circle-trackpads

Wiring
******

This sample uses TM035035-2024 with the Cirque Pinnacle 1CA027 touch
controller connected via SPI. Connect Supply: **VDD 3.3V**, **GND**
and SPI Interface: **SCK**, **SS**, **MOSI**, **MISO** and also
**DR** to an interrupt capable GPIO pin. The supply voltage can be in
the range from 2.5V to 3.3V.

Building and Running
********************

This project outputs sensor data to the console. It requires a 1CA027
based touchpad. It should work with any platform featuring an SPI
peripheral interface. This examples uses the :ref:`nrf52dk_nrf52832`
board.

Building on the nrf52dk_nrf52832 board
======================================

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/pinnacle
   :board: nrf52dk_nrf52832
   :goals: build flash


Sample Output
=============

When a touch is detected a line with coordinates, X, Y and Z separated by
a colon, is printed to the console until the touch disappears and then 20
idle lines are printed.

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-rc2-175-g872f1364ebc8 ***
   1103:892:16
   1103:892:15
   1103:892:16
   1103:892:17
   1106:893:17
   1112:897:16
   1116:902:16
   1117:905:16
   1117:906:15
   1117:907:14
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
   0:0:0
