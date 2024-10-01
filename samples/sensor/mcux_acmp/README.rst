.. zephyr:code-sample:: mcux_acmp
   :name: NXP MCUX Analog Comparator (ACMP)
   :relevant-api: sensor_interface

   Get analog comparator data from an NXP MCUX Analog Comparator (ACMP).

Overview
********

This sample show how to use the NXP MCUX Analog Comparator (ACMP) driver. The
sample supports the :ref:`twr_ke18f`, :ref:`mimxrt1170_evk`, :ref:`frdm_ke17z`
, :ref:`frdm_ke17z512` and :ref:`mimxrt1180_evk`.

The input voltage for the negative input of the analog comparator is
provided by the ACMP Digital-to-Analog Converter (DAC). The input voltage for
the positive input can be adjusted by turning the on-board potentiometer for
:ref:`twr_ke18f` board, for :ref:`mimxrt1170_evk` the voltage signal is
captured on J25-13, the :ref:`frdm_ke17z` and :ref:`frdm_ke17z512` boards are
captured in J2-3, the :ref:`mimxrt1180_evk` board are captured in J45-13, need
change the external voltage signal to check the output.

The output value of the analog comparator is reported on the console.

Building and Running
********************

Building and Running for TWR-KE18F
==================================
Build the application for the :ref:`twr_ke18f` board, and adjust the
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
   :board: mimxrt1170_evk_cm7
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
