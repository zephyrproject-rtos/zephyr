.. zephyr:board:: stm32f769i_eval

Overview
********

The STM32F769I-EVAL board is an evaluation platform for the STM32F769NIH6
microcontroller.

The ST B-LCD40-DSI1 shield can be connected to the board's MIPI DSI interface.
Build an application with the shield using:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: stm32f769i_eval
   :shield: st_b_lcd40_dsi1_mb1166
   :goals: build

Hardware
********

- STM32F769NIH6 Cortex-M7 microcontroller, up to 216 MHz
- 2 MiB flash and 512 KiB internal SRAM
- 32 MiB external SDRAM
- USART1 console
- 8-bit DCMI camera interface
- LTDC and MIPI DSI display interfaces

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board can be flashed with STM32CubeProgrammer. For
example, build and flash the :zephyr:code-sample:`hello_world` application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32f769i_eval
   :goals: build flash
