.. zephyr:board:: stm32h750b_dk

Overview
********

The STM32H750B-DK Discovery kit is a complete demonstration and development
platform for Arm® Cortex®-M7 core-based STM32H750XBH6 microcontroller, with
128Kbytes of Flash memory and 1 Mbytes of SRAM.

The STM32H750B-DK Discovery kit is used as a reference design for user
application development before porting to the final product, thus simplifying
the application development.

The full range of hardware features available on the board helps users to enhance
their application development by an evaluation of all the peripherals (such as
USB OTG FS, Ethernet, microSD™ card, USART, CAN FD, SAI audio DAC stereo with
audio jack input and output, MEMS digital microphone, HyperRAM™,
Octo-SPI Flash memory, RGB interface LCD with capacitive touch panel, and others).
ARDUINO® Uno V3, Pmod™ and STMod+ connectors provide easy connection to extension
shields or daughterboards for specific applications.

STLINK-V3E is integrated into the board, as the embedded in-circuit debugger and
programmer for the STM32 MCU and USB Virtual COM port bridge. STM32H750B-DK board
comes with the STM32CubeH7 MCU Package, which provides an STM32 comprehensive
software HAL library as well as various software examples.

More information about the board can be found at the `STM32H750B-DK website`_.
More information about STM32H750 can be found here:

- `STM32H750 on www.st.com`_
- `STM32H750xx reference manual`_
- `STM32H750xx datasheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

For more details please refer to `STM32H750B-DK website`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_3 TX/RX : PB10/PB11 (ST-Link Virtual Port Com)
- LD1 : PJ2
- LD2 : PI13
- USART1 TX/RX : PB6/PB7 (Arduino D1/D0)

System Clock
============

The STM32H750B System Clock can be driven by an internal or external oscillator,
as well as by the main PLL clock. By default, the System clock
is driven by the PLL clock at 480MHz. PLL clock is feed by a 25MHz high speed external clock.

Serial Port
===========

The STM32H750B Discovery kit has up to 6 UARTs.
The Zephyr console output is assigned to UART3 which connected to the onboard ST-LINK/V3.0. Virtual
COM port interface. Default communication settings are 115200 8N1.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

STM32H750B Discovery kit includes an ST-LINK-V3E embedded debug tool interface.
This probe allows flashing and debugging the board using various tools.

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to write app to the SoC Flash using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Application in SoC Flash
========================

Here is an example for how to build and flash the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h750b_dk
   :goals: build flash

Connect the STM32H750B-DK to your host computer using the ST-LINK
USB port, then run a serial host program to connect with the board. For example:

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyACM0

You should see the following message in the serial host program:

.. code-block:: console

   $ Hello World! stm32h750b_dk

If the application size is too big to fit in SoC Flash,
Zephyr :ref:`Code and Data Relocation <code_data_relocation>` can be used to relocate
the non-critical and big parts of the application to external Flash.

Debugging
---------

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h750b_dk
   :goals: debug

Application in External Flash
=============================

Because of the limited amount of SoC Flash (128KB), you may want to store the application
in external QSPI Flash instead, and run it from there. In that case, the MCUboot bootloader
is needed to chainload the application. A dedicate board variant, ``ext_flash_app``, was created
for this usecase.

:ref:`sysbuild` makes it possible to build and flash all necessary images needed to run a user application
from external Flash.

The following example shows how to build :zephyr:code-sample:`hello_world` with Sysbuild enabled:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: stm32h750b_dk/stm32h750xx/ext_flash_app
   :goals: build
   :west-args: --sysbuild

By default, Sysbuild creates MCUboot and user application images.

Build directory structure created by Sysbuild is different from traditional
Zephyr build. Output is structured by the domain subdirectories:

.. code-block::

  build/
  ├── hello_world
  |    └── zephyr
  │       ├── zephyr.elf
  │       ├── zephyr.hex
  │       ├── zephyr.bin
  │       ├── zephyr.signed.bin
  │       └── zephyr.signed.hex
  ├── mcuboot
  │    └── zephyr
  │       ├── zephyr.elf
  │       ├── zephyr.hex
  │       └── zephyr.bin
  └── domains.yaml

.. note::

   With ``--sysbuild`` option, MCUboot will be re-built every time the pristine build is used,
   but only needs to be flashed once if none of the MCUboot configs are changed.

For more information about the system build please read the :ref:`sysbuild` documentation.

Both MCUboot and user application images can be flashed by running:

.. code-block:: console

   $ west flash

You should see the following message in the serial host program:

.. code-block:: console

   *** Booting MCUboot v2.2.0-173-gb192716c969a ***
   *** Using Zephyr OS build v4.2.0-6260-ge39ba1a35bc4 ***
   I: Starting bootloader
   I: Image index: 0, Swap type: none
   I: Image index: 0, Swap type: none
   I: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
   I: Secondary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
   I: Boot source: none
   I: Image index: 0, Swap type: none
   I: Image index: 0, Swap type: none
   I: Image index: 0, Swap type: none
   I: Image index: 0, Swap type: none
   I: Bootloader chainload address offset: 0x0
   I: Image version: v0.0.0
   I: Jumping to the first image slot
   *** Booting Zephyr OS build v4.2.0-6260-ge39ba1a35bc4 ***
   Hello World! stm32h750b_dk/stm32h750xx/ext_flash_app

To only flash the user application in the subsequent builds, Use:

.. code-block:: console

   $ west flash --domain hello_world

With the default configuration, the board uses MCUboot's Swap-using-offset mode.
To get more information about the different MCUboot operating modes and how to
perform application upgrade, refer to `MCUboot design`_.
To learn more about how to secure the application images stored in external Flash,
refer to `MCUboot Encryption`_.

Debugging
---------

You can debug the application in external flash using ``west`` and ``GDB``.

After flashing MCUboot and the app, execute the following command:

.. code-block:: console

   $ west debugserver

Then, open another terminal (don't forget to activate Zephyr's environment) and execute:

.. code-block:: console

   $ west attach

.. _STM32H750B-DK website:
   https://www.st.com/en/evaluation-tools/stm32h750b-dk.html

.. _STM32H750 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h750-value-line.html

.. _STM32H750xx reference manual:
   https://www.st.com/resource/en/reference_manual/rm0433-stm32h742-stm32h743753-and-stm32h750-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32H750xx datasheet:
   https://www.st.com/resource/en/datasheet/stm32h750ib.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _MCUboot design:
   https://docs.mcuboot.com/design.html

.. _MCUboot Encryption:
   https://docs.mcuboot.com/encrypted_images.html
