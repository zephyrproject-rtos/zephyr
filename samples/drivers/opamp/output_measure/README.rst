.. zephyr:code-sample:: opamp_output_measure
   :name: Operational Amplifier (OPAMP) output measure sample
   :relevant-api: opamp_interface

   Amplify the input of the OPAMP and sample it using the ADC.

Overview
********

This sample demonstrates how to use the :ref:`OPAMP driver API <opamp_api>`.

Building and Running
********************

To build and run this example, users need to configure the following:
1. Configure one channel of the ADC to measure the output of the OPAMP.
2. Use DT to configure the OPAMP. If the OPAMP supports programmable gain,
you can set the gain you want to test using property 'programmable-gain'.

Note:
1. Any gain value should not cause the output of the OPAMP to be
less than the theoretical minimum or greater than the theoretical maximum.
2. The output of the OPAMP should not exceed the range of the ADC.

Building and Running for NXP FRDM-MCXA156
=========================================
Build the application for the :zephyr:board:`frdm_mcxa156` board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/opamp/output_measure
   :board: frdm_mcxa156
   :goals: build flash
   :compact:

Hardware connection (OPAMP0 in differential mode):
1. Positive input channel J2-12 (OPAMP0_INP) connects to a voltage source.
2. Negative input channel R60-3 (OPAMP0_INN) connects to a voltage source.
3. Vout = Vref + (Vinp - Vinn) * Ngain.

Building and Running for NXP FRDM-MCXA346
=========================================
Build the application for the :zephyr:board:`frdm_mcxa346` board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/opamp/output_measure
   :board: frdm_mcxa346
   :goals: build flash
   :compact:

Hardware connection (OPAMP0 in follower mode, OPAMP1 in inverting mode):
1. Positive input channel J4-1 (OPAMP0_INP) connects to GND.
2. Negative input channel J4-3 (OPAMP0_INN) connects to a voltage source.
3. Vout = Vref - Vinn * Ngain.
Note: The frdm_mcxa346 OPAMP does not support programmable gain and its
gain is fixed at 10.

Building and Running for NXP FRDM-MCXA266
=========================================
Build the application for the :zephyr:board:`frdm_mcxa266` board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/opamp/output_measure
   :board: frdm_mcxa266
   :goals: build flash
   :compact:

Hardware connection (OPAMP0 in follower mode, OPAMP1 in inverting mode):
1. Positive input channel J4-1 (OPAMP0_INP) connects to GND.
2. Negative input channel J4-3 (OPAMP0_INN) connects to a voltage source.
3. Vout = Vref - Vinn * Ngain.
Note: The frdm_mcxa266 OPAMP does not support programmable gain and its
gain is fixed at 10.

Building and Running for NXP FRDM-MCXN947
=========================================
Build the application for the :zephyr:board:`frdm_mcxn947` board.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/opamp/output_measure
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: build flash
   :compact:

Hardware connection (OPAMP0 in non-inverting mode):
1. Positive input channel J4-1 (OPAMP0_INP0) connects to a voltage source.
2. Negative input channel J4_3 (OPAMP0_INN) connects to GND.
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

Hardware connection (OPAMP0 in inverting mode):
1. DNP on-board R193, R194.
2. Positive input channel J13-1 (OPAMP0_INP) connect to GND.
3. Negative input channel J13-3 (OPAMP0_INN) connect to a voltage source.
4. Vout = Vref - Ngain * Vinn.

Sample output
=============

You will see the inverting and non-inverting gain and the output of the opamp
in the terminal. Users can calculate the theoretical output value of the opamp
according to the formula and compare it with the ADC measurement value.

The following output is printed:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-2155-gb21dd897e034 ***
   OPAMP output is: 1513(mv)
   OPAMP output is: 907(mv)
   OPAMP output is: 613(mv)

.. note:: The values shown above might differ.
