.. zephyr:board:: gd32f527i_eval

Overview
********

The GD32F527I-EVAL board is a hardware platform for prototyping with the
GigaDevice GD32F527IS Cortex-M33F MCU.

The Zephyr board configuration enables the GD32F527IS SoC, USART0 console,
GPIO, LEDs, user keys, pin control, clock control, and CKOUT0 clock output.

Hardware
********

The following hardware is described by the current Zephyr board files:

- GD32F527IS MCU
- 512 KiB SRAM
- 25 MHz HXTAL and PLL, with a 200 MHz system clock
- LED1 to LED4 connected to PF7, PF8, PE3, and PE2
- WAKEUP key on PA0, TAMPER key on PC13, and USER key on PB14
- USART0 console with TX on PA9 and RX on PA10
- CKOUT0 output on PA8, sourced from PLLP and divided by 4

Supported Features
==================

.. zephyr:board-supported-hw::

Clock Configuration
===================

The 25 MHz HXTAL feeds the PLL, which generates a 200 MHz system clock. The AHB
prescaler is 1, the APB1 prescaler is 4, and the APB2 prescaler is 2.

Default Zephyr Peripheral Mapping
=================================

- LED1 to LED4: PF7, PF8, PE3, PE2 (aliases ``led0`` to ``led3``)
- USER key: PB14 (alias ``sw0``)
- TAMPER key: PC13 (alias ``sw1``)
- WAKEUP key: PA0 (alias ``sw2``)
- USART0 TX/RX: PA9/PA10
- CKOUT0: PA8

Serial Port
===========

The default serial console is USART0. The board pin control configuration
connects USART0 TX to PA9 and USART0 RX to PA10. The default baud rate is
115200 bit/s.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

``west flash`` and ``west debug`` use J-Link by default with the GD32F527IS
device. Connect a J-Link probe to the SWD header of the board.

pyOCD can be used as an alternative runner. The GD32F527 DFP pack is not
included in the GigaDevice HAL module. Download
``GigaDevice.GD32F527_DFP.1.4.0.pack`` from the GD32 MCU website
(https://www.gd32mcu.com) and copy it to
:file:`modules/hal/gigadevice/gd32f527/support/`, which is where the board
runner configuration looks for it. Then install pyOCD, connect a GD-Link or
other CMSIS-DAP probe, and select the runner explicitly:

.. code-block:: console

   west flash --runner pyocd
   west debug --runner pyocd

Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: gd32f527i_eval
   :goals: build
   :compact:

Run your favorite terminal program to listen for output. On Linux the terminal
should be something like ``/dev/ttyUSB0``. For example:

.. code-block:: console

   minicom -D /dev/ttyUSB0 -o

The -o option tells minicom not to send the modem initialization string.
Connection should be configured as follows:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

To flash an image:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: gd32f527i_eval
   :goals: flash
   :compact:

You should see "Hello World! gd32f527i_eval" in your terminal.

To debug an image:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: gd32f527i_eval
   :goals: debug
   :compact:
