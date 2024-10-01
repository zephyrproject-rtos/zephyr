.. zephyr:code-sample:: mcux_lpcmp
   :name: NXP MCUX Low-power Analog Comparator (LPCMP)
   :relevant-api: sensor_interface

   Get analog comparator data from an NXP MCUX Low-power Analog Comparator (LPCMP).

Overview
********

This sample show how to use the NXP MCUX Low-power Analog Comparator (LPCMP) driver.

In this application, the negative input port of the LPCMP is set to 7 which
means the input voltage comes from the LPCMP internal DAC, the reference
voltage of the DAC is set to 0 (for the specific chip, the user needs to
check the reference manual to confirm where this reference voltage comes
from), the output voltage of the DAC is equal to (VREF/256)*(data+1), where
data is set through the attribute ``SENSOR_ATTR_MCUX_LPCMP_DAC_OUTPUT_VOLTAGE``.
The positive input port is set to 0, the user needs to check the reference
manual and board schematic to confirm which specific port is used and can
connect an external voltage to that port and change the input voltage to
see the output change of the LPCMP.

The output value of the LPCMP is reported on the console.

Building and Running
********************

Building and Running for NXP FRDM-MCXN947
=========================================
Build the application for the :ref:`frdm_mcxn947` board, and adjust the
LPCMP positive input port voltage by changing the voltage input to J2-17.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_lpcmp
   :board: frdm_mcxn947//cpu0
   :goals: build flash
   :compact:
