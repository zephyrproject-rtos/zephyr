.. zephyr:code-sample:: mcux_acmp
   :name: NXP MCUX Analog Comparator (ACMP)
   :relevant-api: sensor_interface

   Get analog comparator data from an NXP MCUX Analog Comparator (ACMP).

Overview
********

This sample show how to use the NXP MCUX Analog Comparator (ACMP) driver. The
sample supports the :zephyr:board:`twr_ke18f`, :zephyr:board:`mimxrt1170_evk`, :zephyr:board:`frdm_ke17z`
, :zephyr:board:`frdm_ke17z512`, :zephyr:board:`mimxrt1180_evk` and :zephyr:board:`mimxrt700_evk`.

The input voltage for the negative input of the analog comparator is
provided by the ACMP Digital-to-Analog Converter (DAC). The output value
of the analog comparator is reported on the console.

Building and Running
********************

Building and Running for TWR-KE18F
==================================
Build the application for the :zephyr:board:`twr_ke18f` board, and adjust the
ACMP input voltage by turning the on-board potentiometer.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_acmp
   :board: twr_ke18f
   :goals: flash
   :compact:

Building and Running for MIMXRT1170-EVK
=======================================
Build the application for the MIMXRT1170-EVK board, and adjust the
ACMP input voltage by changing the voltage input to J25-13.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_acmp
   :board: mimxrt1170_evk/mimxrt1176/cm7
   :goals: flash
   :compact:

Building and Running for FRDM-KE17Z
===================================
Build the application for the FRDM-KE17Z board, and adjust the
ACMP input voltage by changing the voltage input to J2-3.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_acmp
   :board: frdm_ke17z
   :goals: flash
   :compact:

Building and Running for FRDM-KE17Z512
======================================
Build the application for the FRDM-KE17Z512 board, and adjust the
ACMP input voltage by changing the voltage input to J2-3.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_acmp
   :board: frdm_ke17z512
   :goals: flash
   :compact:

Building and Running for MIMXRT1180-EVK
=======================================
Build the application for the MIMXRT1180-EVK board, and adjust the
ACMP input voltage by changing the voltage input to J45-13.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_acmp
   :board: mimxrt1180_evk/mimxrt1189/cm33
   :goals: flash
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_acmp
   :board: mimxrt1180_evk/mimxrt1189/cm7
   :goals: flash
   :compact:

Building and Running for MIMXRT700-EVK
=======================================
Build the application for the MIMXRT700-EVK board, and adjust the
ACMP input voltage by changing the voltage input to J7-9.

Note:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_acmp
   :board: mimxrt700_evk/mimxrt798s/cm33_cpu0
   :goals: flash
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mcux_acmp
   :board: mimxrt700_evk/mimxrt798s/cm33_cpu1
   :goals: flash
   :compact:
