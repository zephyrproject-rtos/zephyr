.. _arduino_uno_r4:

Arduino UNO R4 Minima
#####################

Overview
********

The Arduino UNO R4 Minima is a development board featuring the Renesas RA4M1 SoC
in the Arduino form factor and is compatible with traditional Arduino.

Programming and debugging
*************************

Building & Flashing
===================

You can build and flash an application in the usual way (See
:ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_uno_r4_minima
   :goals: build flash

Debugging
=========

Debugging also can be done in the usual way.
The following command is debugging the :zephyr:code-sample:`blinky` application.
Also, see the instructions specific to the debug server that you use.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_uno_r4_minima
   :maybe-skip-config:
   :goals: debug


Using pyOCD
-----------

Various debug adapters, including cmsis-dap probes, can debug the Arduino UNO R4 with pyOCD.
The default configuration uses the pyOCD for debugging.
You must install CMSIS-Pack when flashing or debugging Arduino UNO R4 Minima with pyOCD.
If not installed yet, execute the following command to install CMSIS-Pack for Arduino UNO R4.

.. code-block:: console

   pyocd pack install r7fa4m1ab


Restoring Arduino Bootloader
============================

If you corrupt the Arduino bootloader, you can restore it with the following command.

.. code-block:: console

   wget https://raw.githubusercontent.com/arduino/ArduinoCore-renesas/main/bootloaders/UNO_R4/dfu_minima.hex
   pyocd flash -e sector -a 0x0 -t r7fa4m1ab dfu_minima.hex
