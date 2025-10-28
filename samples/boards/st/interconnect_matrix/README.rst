.. zephyr:code-sample:: stm32_interconnection_matrix
   :name: STM32 interconnect matrix
   :relevant-api: pwm_interface

   Devicetree configuration for generation of timer events.

Overview
********

Devicetree configuration for event generation on the interconnect
matrix by the timer module (TRGO) of an STM32 device.
The :ref:`PWM API <pwm_api>` is used to configure the PWM signal.

Requirements
************

This sample requires the support of a timer master mode compatible timer block.

Building and Running
********************

 .. zephyr-app-commands::
   :zephyr-app: samples/boards/st/interconnect_matrix
   :board: nucleo_g070rb
   :goals: build
   :compact:
