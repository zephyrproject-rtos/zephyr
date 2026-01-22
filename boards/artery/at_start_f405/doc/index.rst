.. zephyr:board:: at_start_f405

Overview
********

The AT START F405 board features an ARM Cortex-M4 based AT32F405 MCU
with a wide range of connectivity support and configurations.

Hardware
********

- ARM Cortex-M4F Processor
- Core clock up to 216 MHz
- 256KB Flash memory
- 96 KB SRAM
- 1x12-bit 2MSPS ADC
- Up to 6x USART and 2x UART
- Up to 3x I2C
- Up to 3x SPI
- 1x QSPI interface
- 1x CAN interface(2.0B Active)
- 1x OTGHS on chip phy, Support usb2.0 high speed
- 1x OTGFS Support usb2.0 Full speed
- Up to 14 times
- 2 x 7-channel DMA controllers

Supported Features
==================

.. zephyr:board-supported-hw::

Serial Port
===========

The AT-START-F405 board has one serial communication port. The default port
is USART1 with TX connected at PA9 and RX at PA10.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Using ATLink or J-Link
=======================
The board comes with an embedded AT-Link programmer.
You need to install CMSIS-Pack which is required by pyOCD
when programming or debugging by the AT-Link programmer.
Execute the following command to install CMSIS-Pack for AT32F405CCT7
if not installed yet.

   .. code-block:: console

      pyocd pack install at32f405cct7

Also, J-Link can be used to program the board via the SWD interface
(PA13/SWDIO and PA14/SWCLK).

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/blinky
      :board: at_start_f405
      :goals: build
      :compact:

#. To flash an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/blinky
      :board: at_start_f405
      :goals: flash
      :compact:

   When using J-Link, append ``--runner jlink`` option after ``west flash``.

   You should see blink LED2 on board AT-START-F405.

#. To debug an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/blinky
      :board: at_start_f405
      :goals: debug
      :compact:

   When using J-Link, append ``--runner jlink`` option after ``west debug``.

References
**********

.. _artery technology website: https://www.arterychip.com/en/product/AT32F405.jsp
