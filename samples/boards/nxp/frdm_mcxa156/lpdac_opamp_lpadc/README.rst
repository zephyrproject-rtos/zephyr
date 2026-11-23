.. zephyr:code-sample:: frdm_mcxa156_lpdac_opamp_lpadc
   :name: FRDM-MCXA156 lpdac opamp lpadc example
   :relevant-api: adc_interface

   Amplify the input of the OPAMP and sample it using the ADC.

Overview
********

In this example:
1. The LPDAC outputs a 300mv voltage to the non-inverting terminal of the OPAMP.

2. The OPAMP operates in the non-inverting amplifier mode, the inverting terminal
of the OPAMP is grounded.

3. The channel 0 of the LPADC is connected to the output of the OPAMP, and the
channel 1 of the LPADC is connected to the output of the DAC (the non-inverting
input terminal of the OPAMP).

4. The ideal sampling range of OPAMP output channel is specified by
'ideal-sample-range', and 'nxp,opamps' is used to specify LPADC samples which
OPAMP's output and which ADC channel is connected to the OPAMP output.

5. The OPAMP non-inverting input is 300mV, and the OPAMP gain is initialized to 1.
Therefore, the first round OPAMP output (~300mV) will not reach the minimum value
of the 'ideal-sampling-range'. Therefore, the ADC driver will increase the OPAMP
gain through 'opamp_set_gain'. The second round OPAMP output (~900mV) is still
below the minimum value, so the gain will continue to increase. The third round
OPAMP output (~1500mV) is already in the ideal range, so the gain will not be
adjusted later.

Building and Running
********************
Build the application for the :zephyr:board:`frdm_mcxa156` board.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/frdm_mcxa156/lpdac_opamp_lpadc
   :board: frdm_mcxa156
   :goals: build flash
   :compact:

Hardware connection (OPAMP0 in differential mode):
1. OPAMP positive input channel J2-12 connects to DAC0 output J2-9, and
LPADC0 channel 1 input J4-2.
2. OPAMP negative input channel R60-3 connects to GND.
4. Vout = Vinp * (Ngain + 1), if Ngain > 1; Vout = Vinp * Ngain, if Ngain = 1.

Sample output
=============

You will see the raw data and voltage of the opamp non-inverting
input (DAC output voltage) and the raw data and voltage of the
opamp output in the terminal.

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-3573-gd796bf703856 ***
   round 0: output=5796 (~291 mV), input=5949 (~299 mV)
   round 1: output=17806 (~896 mV), input=5945 (~299 mV)
   round 2: output=29506 (~1485 mV), input=5964 (~300 mV)
   round 3: output=29508 (~1485 mV), input=5957 (~299 mV)
   round 4: output=29529 (~1486 mV), input=5944 (~299 mV)

.. note:: The values shown above might differ.
