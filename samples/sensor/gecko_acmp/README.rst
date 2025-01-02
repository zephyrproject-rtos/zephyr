.. zephyr:code-sample:: gecko_acmp
   :name: SiLabs Gecko Analog Comparator (ACMP)
   :relevant-api: sensor_interface

   Get analog comparator data from an SiLabs Gecko Analog Comparator (ACMP).

Overview
********

This sample show how to use the SiLabs Gecko Analog Comparator (ACMP) driver.

In this application, the negative input port of the ACMP is muxed to the internal
low-power reference voltage 1.25V.
The positive input port is muxed to VDD (VSENSE01DIV4LP) divided by 4.
The threshold value is set to 20h (32d).
Corresponding voltage threshold VREFOUT = VREFIN * (VREFDIV / 63) = (1.25V * 4) * (32 / 63) = 2.54V.
The ACMP output shall be set to 1 when VDD voltage is under 2.54V and is reported on the console.

Building and Running
********************

Building and Running for SiLabs EFR32MG24 Mighty Gecko Board dev kit
=========================================
Build the application for the :zephyr:board:`xg24_dk2601b` board, and adjust the
ACMP positive/negative input ports and vrefdiv in xg24_dk2601b.overlay file directly.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/gecko_acmp
   :board: xg24_dk2601b
   :goals: build flash
   :compact:
