.. zephyr:code-sample:: opamp_output_measure
   :name: Operational Amplifier (OPAMP)
   :relevant-api: opamp_interface

   Amplify the input of the OPAMP and sample it using the ADC.

Overview
********

This sample demonstrates how to use the :ref:`OPAMP driver API <opamp_api>`.

Building and Running
********************

To build and run this example, users need to configure the following:
1. Configure one channel of the ADC to measure the output of the OPAMP.
2. Specify the inverting and non-inverting gain in
:zephyr_file:`samples/drivers/opamp/output_measure/src/main.h`. Note that
any gain value should not cause the output of the OPAMP to be less than
the theoretical minimum or greater than the theoretical maximum.

Building and Running for NXP FRDM-MCXA156
=========================================
Build the application for the :zephyr:board:`frdm_mcxa156` board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/opamp/output_measure
   :board: frdm_mcxa156
   :goals: build flash
   :compact:

Hardware connection:
1. Positive input channel J2-12 (OPAMP0_INP) connects to a voltage source.
2. Negative input channel R60-3 (OPAMP0_INN) connects to a voltage source.
3. Vout = Vref + (Vinp - Vinn) * Ngain.

Building and Running for NXP FRDM-MCXA166
=========================================
Build the application for the :zephyr:board:`frdm_mcxa166` board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/opamp/output_measure
   :board: frdm_mcxa166
   :goals: build flash
   :compact:

Hardware connection:
1. Positive input channel J4-1 (OPAMP0_INP) connects to GND.
2. Negative input channel J4-3 (OPAMP0_INN) connects to a voltage source.
3. Vout = Vref - Vinn * Ngain.

Building and Running for NXP FRDM-MCXA276
=========================================
Build the application for the :zephyr:board:`frdm_mcxa276` board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/opamp/output_measure
   :board: frdm_mcxa276
   :goals: build flash
   :compact:

Hardware connection:
1. Positive input channel J4-1 (OPAMP0_INP) connects to GND.
2. Negative input channel J4-3 (OPAMP0_INN) connects to a voltage source.
3. Vout = Vref - Vinn * Ngain.

Building and Running for NXP FRDM-MCXN947
=========================================
Build the application for the :zephyr:board:`frdm_mcxn947` board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/opamp/output_measure
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: build flash
   :compact:

Hardware connection:
1. Positive input channel J4-1 (OPAMP0_INP) connects to a voltage source.
2. Negative input channel J4_3 (OPAMP0_INN) connects to a voltage source.
3. Vout = Vref + (Vinp - Vinn) * Ngain.

Building and Running for NXP LPCXpresso55S36
============================================
The sample can be built and executed for the :zephyr:board:`lpcxpresso55s36` as
follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/opamp/output_measure
   :board: lpcxpresso55s36
   :goals: build flash
   :compact:

Hardware connection:
1. DNP on-board R913, R194.
2. Positive input channel J13-5 (OPAMP0_INP) connect to GND.
3. Negative input channel J13-3 (OPAMP0_INN) connect to a voltage source.
4. Vout = Vref - Ngain * Vinn.

Sample output
=============

You will see the inverting and non-inverting gain and the output of the opamp
in the terminal. Users can calculate the theoretical output value of the opamp
according to the formula and compare it with the ADC measurement value.

The following output is printed:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-1004-g26c31c638c9b ***
   Set opamp inverting gain to: 1
   Set opamp non-inverting gain to: 1
   OPAMP output is: 1124(mv)

.. note:: The values shown above might differ.
