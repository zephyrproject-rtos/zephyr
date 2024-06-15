.. zephyr:code-sample:: cmsis-dsp-moving-average
   :name: CMSIS-DSP moving average

   Use the CMSIS-DSP library to calculate the moving average of a signal.

Overview
********

This sample demonstrates how to use the CMSIS-DSP library to calculate the moving average of a
signal.

It can be run on any board supported in Zephyr, but note that CMSIS-DSP is specifically optimized
for ARM Cortex-A and Cortex-M processors.

A **moving average** filter is a common method used for smoothing noisy data. It can be implemented
as a finite impulse response (FIR) filter where the filter coefficients are all equal to 1/N, where
N is the number of "taps" (i.e. the size of the moving average window).

The sample uses a very simple input signal of 32 samples, and computes the moving average using a
"window" of 10 samples. The resulting output is computed in one single call to the ``arm_fir_q31()``
CMSIS-DSP function, and displayed on the console.

.. note::
   In order to allow an easy comparison of the efficiency of the CMSIS-DSP library when used on ARM
   processors vs. other architectures, the sample outputs the time and number of cycles it took to
   compute the moving average.

Requirements
************

CMSIS-DSP is an optional module and needs to be added explicitly to your Zephyr workspace:

.. code-block:: shell

   west config manifest.project-filter -- +cmsis-dsp
   west update cmsis-dsp

Building and Running
*********************

The demo can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/cmsis-dsp/moving_average
   :host-os: unix
   :board: qemu_cortex_m0
   :goals: run
   :compact:

The sample will output the number of cycles it took to compute the moving averages, as well as the
computed average for each 10-sample long window of the input signal.

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-224-gb55824751d6c ***
   Time: 244 us (244 cycles)
   Input[00]:  0  0  0  0  0  0  0  0  0  0 | Output[00]:   0.00
   Input[01]:  0  0  0  0  0  0  0  0  0  1 | Output[01]:   0.10
   Input[02]:  0  0  0  0  0  0  0  0  1  2 | Output[02]:   0.30
   Input[03]:  0  0  0  0  0  0  0  1  2  3 | Output[03]:   0.60
   ...
   Input[30]: 21 22 23 24 25 26 27 28 29 30 | Output[30]:  25.50
   Input[31]: 22 23 24 25 26 27 28 29 30 31 | Output[31]:  26.50
