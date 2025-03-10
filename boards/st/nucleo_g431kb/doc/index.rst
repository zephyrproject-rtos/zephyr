.. zephyr:board:: nucleo_g431kb

Overview
********

The Nucleo G431KB board features an ARM Cortex-M4 based STM32G431KB MCU
with a wide range of connectivity support and configurations.
Here are some highlights of the Nucleo G431KB board:

- STM32 microcontroller in LQFP32 package
- Arduino Nano V3 connectivity
- On-board ST-LINK/V3E debugger/programmer
- Flexible board power supply:

  - USB VBUS or external source(3.3 V, 5 V, 7 - 12 V)
  - Power management access point

- Three LEDs: USB communication (LD1), power LED (LD3), user LED (LD2)
- One push-button for RESET

More information about the board can be found at the `Nucleo G431KB website`_.

- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell.

More information about STM32G431KB can be found here:

- `STM32G431KB on www.st.com`_
- `STM32G4 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Nucleo G431KB Board has 6 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `STM32G4 Nucleo-32 board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- LPUART_1_TX : PA2
- LPUART_1_RX : PA3
- LD2 : PB8
- PWM_4_CH_3 : PB8
- I2C_2_SCL : PA9
- I2C_2_SDA : PA8

System Clock
------------

The Nucleo G431KB System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default the external oscillator is not connected to the board. Therefore only the internal
High Speed oscillator is supported. By default System clock is driven by PLL clock at 170 MHz,
the PLL is driven by the 16 MHz high speed internal oscillator.

Serial Port
-----------

Nucleo G431KB board has 1 U(S)ARTs and one LPUART. The Zephyr console output is assigned to LPUART1.
Default settings are 115200 8N1.

Please note that LPUART1 baudrate is limited to 9600 if the MCU is clocked by LSE (32.768 kHz) in
low power mode.

Programming and Debugging
*************************

Nucleo G431KB Board includes an ST-Link/V3 embedded debug tool interface.

Applications for the ``nucleo_g431kb`` board target can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD, or pyOCD can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner pyocd

To enable support of the STM32G431KB SoC in pyOCD, its pack has to be installed first:

.. code-block:: console

   $ pyocd pack --update
   $ pyocd pack --install stm32g431kb

Flashing an application to Nucleo G431KB
----------------------------------------

Connect the Nucleo G431KB to your host computer using the USB port,
then run a serial host program to connect with your Nucleo board.

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Now build and flash an application. Here is an example for
:zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_g431kb
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   $ Hello World! nucleo_g431kb/stm32g431xx

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_g431kb
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _Nucleo G431KB website:
    https://www.st.com/en/evaluation-tools/nucleo-g431kb.html

.. _STM32G4 Nucleo-32 board User Manual:
   https://www.st.com/resource/en/user_manual/um2397-stm32g4-nucleo32-board-mb1430-stmicroelectronics.pdf

.. _STM32g431kb Nucleo-32 board schematic:
    https://www.st.com/resource/en/schematic_pack/mb1430-g431kbt6-a02_schematic_internal.pdf

.. _STM32G431KB on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g431kb.html

.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
