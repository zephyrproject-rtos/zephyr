.. zephyr:code-sample:: stm32_pwm_mastermode
   :name: STM32 pwm mastermode
   :relevant-api: pwm_interface

   Devicetree configuration for utilizing master/slave capabilities
   to synchronize timer instances.

Overview
********

Devicetree configuration for event generation on the interconnect
matrix by the timer module (master mode selection / trigger output
TRGO) and the usage of these triggers for timer synchronization (slave
mode / trigger input) of an STM32 device.
The :ref:`PWM API <pwm_api>` is used to configure the PWM signal.

Requirements
************

This sample requires the support of two master/slave mode-compatible timer
blocks.

Building and Running
********************

 .. zephyr-app-commands::
   :zephyr-app: samples/boards/st/pwm/mastermode
   :board: nucleo_g070rb
   :goals: build
   :compact:
