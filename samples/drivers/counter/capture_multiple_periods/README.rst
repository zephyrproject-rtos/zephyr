.. zephyr:code-sample:: counter-capture-multiple-periods
   :name: Capture multiple signal periods
   :relevant-api: counter_interface pwm_interface

   Measure the periods of two independent signals with two capture channels
   sharing one free-running counter.

Overview
********

This sample configures two PWM generators with different periods and captures
their rising edges on channels 1 and 2 of one timer counter. Because both
capture channels latch the same free-running counter, the signals are measured
concurrently in one timestamp domain.

DMA transfers each channel's captured timestamps into an independent circular
buffer split into two equal halves. The default batch size is 128 timestamps.
The default capture buffer size is 256 timestamps. The DMA callback returns one
128-timestamp half-buffer at a time while DMA continues filling the other half.
The total buffer size is set by ``CAPTURE_BUFFER_SIZE`` in the sample source.
Period calculation and printing run in thread context.

Wiring
******

On the NUCLEO-C5A3ZG, connect the following pins:

* PB10 (TIM2_CH3 generator) to PA8 (TIM1_CH1 capture)
* PC6 (TIM8_CH1 generator) to PA9 (TIM1_CH2 capture)

Building and Running
********************

Build the sample for the NUCLEO-C5A3ZG:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/counter/capture_multiple_periods
   :board: nucleo_c5a3zg
   :goals: build flash
   :compact:

The output reports the average period and frequency measured independently on
both input channels. The board overlays configure the timer and DMA routing
required by each board.
