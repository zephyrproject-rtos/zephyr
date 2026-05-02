.. zephyr:board:: stm32f0_disco

Overview
********

The STM32F0 Discovery development board uses an STM32F051R8T6 MCU and
integrates the ST-LINK/V2-1 debugger and programmer.  It also comes with a
comprehensive STM32 software HAL library and various packaged software
examples.

More information about the board can be found at the `STM32F0DISCOVERY website`_.

Hardware
********

The STM32 Discovery board features:

- STM32F051R8T6 microcontroller featuring 64 KB Flash memory, 8 KB RAM in an
  LQFP64 package
- On-board ST-LINK/V2 with selection mode switch to use the kit as a standalone
  ST-LINK/V2 (with SWD connector for programming and debugging)
- Board power supply: through USB bus or from an external 5 V supply voltage
- External application power supply: 3 V and 5 V
- Four LEDs:

    - LD1 (red) for 3.3 V power on
    - LD2 (red/green) for USB communication
    - LD3 (green) for PC9 output
    - LD4 (blue) for PC8 output
- Two push buttons (user and reset)
- Extension header for all LQFP64 I/Os for quick connection to prototyping board
  and easy probing
- An additional board is provided which can be connected to the extension
  connector for even easier prototyping and probing.
- Comprehensive free software including a variety of examples, part of
  STM32CubeF0 package or STSW-STM32049 for legacy Standard Libraries usage

More information about STM32F051R8 can be found in the `STM32F0x8 reference manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Each of the GPIO pins can be configured by software as output (push-pull or open-drain), as
input (with or without pull-up or pull-down), or as peripheral alternate function. Most of the
GPIO pins are shared with digital or analog alternate functions. All GPIOs are high current
capable except for analog inputs.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1_TX : PA9
- UART_1_RX : PA10
- UART_2_TX : PA2
- UART_2_RX : PA3

For more details please refer to `STM32F0DISCOVERY board User Manual`_.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

STM32F0DISCOVERY board includes an ST-LINK/V2-1 embedded debug tool interface.

Applications for the ``stm32f0_disco`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Flashing an application to Nucleo F030R8
----------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32f0_disco
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32f0_disco
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _STM32F0DISCOVERY website:
   https://www.st.com/en/evaluation-tools/stm32f0discovery.html

.. _STM32F0x8 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00031936.pdf

.. _STM32F0DISCOVERY board User Manual:
   https://www.st.com/resource/en/user_manual/dm00050135.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
