.. _mcux_acmp:

NXP MCUX Analog Comparator (ACMP)
#################################

Overview
********

This sample show how to use the NXP MCUX Analog Comparator (ACMP) driver. The
sample supports the :ref:`twr_ke18f`.

The input voltage for the the negative input of the analog comparator is
provided by the ACMP Digital-to-Analog Converter (DAC). The input voltage for
the positive input can be adjusted by turning the on-board potentiometer.

The output value of the analog comparator is reported on the console.

Building and Running
********************

Build the application for the :ref:`twr_ke18f` board, and adjust the
ACMP input voltage by turning the on-board potentiometer.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/mcux_acmp
   :board: twr_ke18f
   :goals: flash
   :compact:
