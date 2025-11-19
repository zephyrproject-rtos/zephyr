.. zephyr:code-sample:: stm32_timer_mastermode
   :name: STM32 timer mastermode
   :relevant-api: pwm_interface

   Devicetree configuration for generation of timer events.

Overview
********

Devicetree configuration for event generation on the interconnect
matrix by the timer module (master mode selection / trigger output
TRGO) of an STM32 device.
The :ref:`PWM API <pwm_api>` is used to configure the PWM signal.

Requirements
************

This sample requires the support of a timer master mode compatible timer block.

Building and Running
********************

 .. zephyr-app-commands::
   :zephyr-app: samples/boards/st/timer_mastermode
   :board: nucleo_g070rb
   :goals: build
   :compact:
