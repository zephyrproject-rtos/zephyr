.. zephyr:code-sample:: pwm-circular-dma
   :name: PWM circular mode with DMA
   :relevant-api: pwm

   Output two different patterns on two pwm outputs using a circular DMA configuration.

Overview
********

This sample demonstrates how to use STM32 PWM driver with DMA in circular mode.
It takes data from tables and outputs it on two different PWM outputs.


Building and Running
********************

Build and flash the sample as follows, changing ``nucleo_f412zg`` for
your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/pwm/circular_dma
   :board: nucleo_f412zg
   :goals: build flash
   :compact:

Sample Output
=============

On the nucleo_f412zg board two leds are breathing ( change intensity 
periodically).
On other nucleo boards the led breathes and the nArduino D6 pin
is used for the second PWM output
