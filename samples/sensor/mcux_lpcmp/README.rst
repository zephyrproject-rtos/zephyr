.. zephyr:code-sample:: mcux_lpcmp
   :name: NXP MCUX Low-power Analog Comparator (LPCMP)
   :relevant-api: sensor_interface

   Get analog comparator data from an NXP MCUX Low-power Analog Comparator (LPCMP).

Overview
********

This sample show how to use the NXP MCUX Low-power Analog Comparator (LPCMP) driver.

In this application, the negative input port of the LPCMP is set with
:kconfig:option:`CONFIG_LPCMP_NEGATIVE_PORT` which means the input voltage comes
from the LPCMP internal DAC. The reference voltage of the DAC is set to 0 (check
the reference manual to confirm the voltage source for your specific chip). The
output voltage of the DAC equals (VREF/256)*(data+1), where data is set through
the attribute ``SENSOR_ATTR_MCUX_LPCMP_DAC_OUTPUT_VOLTAGE``. The positive input
port is set with :kconfig:option:`CONFIG_LPCMP_POSITIVE_PORT`. Check the reference
manual and board schematic to confirm which specific port is used. You can connect
an external voltage to that port and change the input voltage to see the output
change of the LPCMP.

The output value of the LPCMP is reported on the console.

Building and Running
********************

Building and Running for NXP FRDM-MCXN947
=========================================
Build the application for the :zephyr:board:`frdm_mcxn947` board, and adjust the
LPCMP positive input port voltage by changing the voltage input to J2-17.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_lpcmp
   :board: frdm_mcxn947//cpu0
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXN236
=========================================
Build the application for the :zephyr:board:`frdm_mcxn236` board, and adjust the
LPCMP positive input port voltage by changing the voltage input to J2-8.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_lpcmp
   :board: frdm_mcxn236
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXA156
=========================================
Build the application for the :zephyr:board:`frdm_mcxa156` board, and adjust the
LPCMP positive input port voltage by changing the voltage input to J2-9.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_lpcmp
   :board: frdm_mcxa156
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXA153
=========================================
Build the application for the :zephyr:board:`frdm_mcxa153` board, and adjust the
LPCMP positive input port voltage by changing the voltage input to J2-9.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_lpcmp
   :board: frdm_mcxa153
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXA346
=========================================
Build the application for the :zephyr:board:`frdm_mcxa346` board, and adjust the
LPCMP positive input port voltage by changing the voltage input to J2-17.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_lpcmp
   :board: frdm_mcxa346
   :goals: build flash
   :compact:

Building and Running for NXP FRDM-MCXA266
=========================================
Build the application for the :zephyr:board:`frdm_mcxa266` board, and adjust the
LPCMP positive input port voltage by changing the voltage input to J2-17.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_lpcmp
   :board: frdm_mcxa266
   :goals: build flash
   :compact:
