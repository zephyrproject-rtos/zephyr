.. zephyr:code-sample:: stm32_syscfg_itline
    :name: STM32 SYSCFG IT line

    Show how to use SYSCFG for interrupts on STM32.

Overview
********

This sample shows how to setup board DTS and project config to use SYSCFG for
interrupts on STM32. Example is using NUCLEO-G0B1RE board.

In a dedicated DTS overlay, enable CAN (1, 2), I2C (2, 3), and SPI (2, 3)
devices, that are sharing interrupt lines on STM32G0. Override interrupt setup
to rely on SYSCFG IT line instead of first level interrupts that would be
shared. Relevant SYSCFG IT line has to be set as interrupt parent, and IT bit
offset within the line as interrupt number.

In the project config, setup IRQ aggregators according to number of SYSCFG IT
lines that are enabled, and maximum number of bits within them. STM32 SYSCFG
driver has to be enabled. Support for shared interrupts can be disabled, if all
interrupts sharing lines are handled via SYSCFG IT lines.

Building and Running
********************

This application can be built as follows:

.. zephyr-app-commands::
    :zephyr-app: samples/boards/st/stm32_syscfg_itline
    :goals: build
