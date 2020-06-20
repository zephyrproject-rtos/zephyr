.. _dac-sample:

Digital-to-Analog Converter (DAC)
#################################

Overview
********

This sample demonstrates how to use the DAC driver API.

Building and Running
********************

The DAC output is defined in the board's devicetree and pinmux file.

Building and Running for ST Nucleo L073RZ
=========================================
The sample can be built and executed for the
:ref:`nucleo_l073rz_board` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: nucleo_l073rz
   :goals: build flash
   :compact:

Building and Running for ST Nucleo L152RE
=========================================
The sample can be built and executed for the
:ref:`nucleo_l152re_board` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: nucleo_l152re
   :goals: build flash
   :compact:

Building and Running for NXP TWR-KE18F
======================================
The sample can be built and executed for the :ref:`twr_ke18f` as
follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: twr_ke18f
   :goals: build flash
   :compact:

DAC output is available on pin A32 of the primary TWR elevator
connector.

Building and Running for NXP FRDM-K64F
======================================
The sample can be built and executed for the :ref:`frdm_k64f` as
follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: frdm_k64f
   :goals: build flash
   :compact:

DAC output is available on connector J4 pin 11.

Sample output
=============

You should see a sawtooth signal with an amplitude of the DAC reference
voltage and a period of approx. 4 seconds at the DAC output pin specified
by the board.

The following output is printed:

.. code-block:: console

   Generating sawtooth signal at DAC channel 1.

.. note:: If the DAC is not supported, the output will be an error message.
