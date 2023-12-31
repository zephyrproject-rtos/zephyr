.. _stm32-pm-wkup-pins-gpio-sample:

STM32 Wake-up Pins GPIO Demo
############################

Overview
********

This sample is a minimum application to demonstrate using a wake-up pin with a GPIO as
a wake-up source to exit shutdown mode.

The system will power off automatically ``WAIT_TIME_US`` us after boot or reset.
Press the user button to exit shutdown mode.

.. _stm32-pm-wkup-pins-gpio-sample-requirements:

Requirements
************

The board should support POWEROFF functionality & have wake-up pin that corresponds
to the user button GPIO.
To support another board, add an overlay in boards folder & make sure that wake-pins
are configured in DT.

Building and Running
********************

Build and flash wkup_pins_gpio as follows, changing ``nucleo_u5a5zj_q`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/power_mgmt/wkup_pins_gpio
   :board: nucleo_u5a5zj_q
   :goals: build flash
   :compact:

After flashing, the LED in ON.
Press the user button to exit from shutdown mode.
