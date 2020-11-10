.. _pinetime_devkit0:

Pine64 PineTime DevKit0
#######################

Overview
********

.. figure:: img/PineTime_leaflet.jpg
   :width: 600px
   :align: center
   :alt: Pine64 PineTime

   Board Name (Credit: <owner>)


The PINE64 SmartWatch, dubbed "PineTime", is a product of a community effort
for an open source smartwatch in collaboration with wearable RTOS and Linux
app developers/communities.

.. figure:: img/PineTime_DevKit0.jpg
   :width: 600px
   :align: center
   :alt: Pine64 PineTime

   Board Name (Credit: <owner>)

Hardware
********

The Pinetime is based on a Nordic NRF52832 chip and features:

- 64 MHz Cortex-M4 with FPU
- 64KB SRAM
- 512KB on board Flash
- 1.3 inches (33mm), 240x240 pixels display with ST7789 driver
- 170-180mAh LiPo battery
- XT25F32B 32Mb(4MB) SPI NOR Flash
- CST816S Capacitive Touch
- BMA421 Triaxial VAcceleration Sensor
- HRS3300 PPG Heart Rate Sensor

PineTime Port Assignment
========================

See `Pinetime schematics`_
+----------------------+---------------------------------+-----------+
| NRF52 pins           | Function                        | Direction |
+======================+=================================+===========+
| P0.00/XL1            | 32.768KHz –XL1                  |           |
| P0.01/XL2            | 32.768KHz –XL1                  |           |
| P0.02/AIN0           | SPI-SCK, LCD_SCK                | OUT       |
| P0.03/AIN1           | SPI-MOSI, LCD_SDI               | OUT       |
| P0.04/AIN2           | SPI-MISO                        | IN        |
| P0.05/AIN3           | SPI-CE# (SPI-NOR)               | OUT       |
| P0.06                | BMA421-SDA, HRS3300-SDA, TP-SDA | I/O       |
| P0.07                | BMA421-SCL, HRS3300-SCL, TP-SCL | OUT       |
| P0.08                | BMA421-INT                      | IN        |
| P0.09/NFC1           | LCD_DET                         | OUT       |
| P0.10/NFC2           | TP_RESET                        | OUT       |
| P0.11                |                                 |           |
| P0.12                | CHARGE INDICATION               | IN        |
| P0.13                | PUSH BUTTON_IN                  | IN        |
| P0.14/TRACEDATA3     | LCD_BACKLIGHT_LOW               | OUT       |
| P0.15/TRACEDATA2     | PUSH BUTTON_OUT                 | OUT       |
| P0.16/TRACEDATA1     | VIBRATOR OUT                    | OUT       |
| P0.17                |                                 |           |
| P0.18/TRACEDATA0/SWO | LCD_RS OUT                      |           |
| P0.19                | POWER PRESENCE INDICATION       | IN        |
| P0.20/TRACECLK       |                                 |           |
| P0.21/nRESET         |                                 |           |
| P0.22                | LCD_BACKLIGHT_MID               | OUT       |
| P0.23                | LCD_BACKLIGHT_HIGH              | OUT       |
| P0.24                | 3V3 POWER CONTROL               | OUT       |
| P0.25                | LCD_CS                          | OUT       |
| P0.26                | LCD_RESET                       | OUT       |
| P0.27                | STATUS LED (NOT STAFF)          | OUT       |
| P0.28/AIN4           | TP_INT                          | IN        |
| P0.29/AIN5           |                                 |           |
| P0.30/AIN6           | HRS3300-TEST                    | IN        |
| P0.31/AIN7           | BATTERY VOLTAGE (Analog)        | IN        |
+----------------------+---------------------------------+-----------+

Building
********

In order to build Zephyr for the Pinetime, you can specify the pinetime board
using the -b option:

.. code-block:: console

   $ west build -b pinetime


Programming and Debugging
*************************

More infos to be found there:
      - `Wiki Regrogramming the PineTime`_

The PineTime Dev Kit comes with the back not glued down to allow it to be
easily reprogrammed, however the kit does not include an hardware
/debugger.
There is a bewildering variety of different hardware programmers available
but whatever programmer you have there are only a few tasks you will have to
learn about:
- Unlock the device
- Upload new software
- Run a debugger

Unlocking the device is a one-time action that is needed to enable to debug
port and provide full access to the device. Unlocking the device will erase
all existing software from the internal flash.

Debugger connection
===================

The devkits have exposed SWD pins for flashing and debugging.

Only a few devs have soldered to these pins, most just use friction to make
contact with the programming cable.
The pinout is:

.. figure:: img/PineTime_SWD_location.jpg
   :width: 300px
   :align: center
   :alt: PineTime SWD location

Unlocking the Flash memory
==========================

Unlocking the device and erase the memory.

You need to execute this step only once, to remove the read protection on the
memory. Note that it will erase the whole flash memory of the MCU!:

.. code-block:: console

   $ nrfjprog -f NRF52 --recover

Flashing
========

Using nrfjprog, flashing the Pinetime is done wit hthe command:

.. code-block:: console

   $ nrfjprog -f NRF52 --program firmware.hex --sectorerase

Debugging
=========
Using Segger Ozone debugger, debugging and flashing is made easy.

Simply load the .elf file containing the final firmware and
setup the debbuger to use SWD ober USB for the chip nRF52832_xxAA.
This setup can be done using the menu Tools/J-Link Settings. or directly type
in the debugger console the folowing:

.. code-block:: console

   $ Project.SetDevice ("nRF52832_xxAA");
   $ Project.SetHostIF ("USB", "");
   $ Project.SetTargetIF ("SWD");
   $ Project.SetTIFSpeed ("4 MHz");
   $ File.Open ("/Users/sdorre/dev/nrf52/pinetine-hypnos/pinetime/build/zephyr/zephyr.elf");

References
**********

.. target-notes::

.. _Pine64 Pinetime presentation: https://www.pine64.org/pinetime
.. _Pine64 wiki: https://wiki.pine64.org/index.php/PineTime
.. _Pine64 forum: https://forum.pine64.org
.. _Pinetime schematics:
   http://files.pine64.org/doc/PineTime/PineTime%20Schematic-V1.0a-20191103.pdf
.. _Wiki Regrogramming the PineTime:
   https://wiki.pine64.org/index.php/Reprogramming_the_PineTime
