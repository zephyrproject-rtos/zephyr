.. zephyr:board:: art_pi2

Overview
********

The ART-Pi2 is an open-source hardware platform designed by the
RT-Thread team specifically for embedded software engineers
and open-source makers, offering extensive expandability for DIY projects.

Key Features

- STM32H7R7L8HxH microcontroller featuring 64 Kbytes of Flash and 620 Kbytes of SRAM in an TFBGA225 package
- On-board ST-LINK/V2.1 debugger/programmer
- SDIO TF Card slot
- SDIO WIFI:CYWL6208
- HDC UART BuleTooth:CYWL6208
- 32-MB HyperRAM
- 64-MB HyperFlash
- One Power LED (blue) for 3.3 V power-on
- Two user LEDs blue and red
- Two ST-LINK LEDs: blue and red
- Two push-buttons (user and reset)
- Board connectors:

  - USB OTG with Type-C connector
  - RGB888 FPC connector

More information about the board can be found at the `ART-Pi2 website`_.

Hardware
********

ART-Pi2 provides the following hardware components:

The STM32H7R7xx devices are a high-performance microcontrollers family (STM32H7
Series) based on the high-performance Arm |reg| Cortex |reg|-M7 32-bit RISC core.
They operate at a frequency of up to 600 MHz.

More information about STM32H7R7 can be found here:

- `STM32H7R7L8 on www.st.com`_
- `STM32H7Rx reference manual`_


Supported Features
==================

.. zephyr:board-supported-hw::

Default Zephyr Peripheral Mapping:
----------------------------------

The ART-Pi2 board features a On-board ST-LINK/V2.1 debugger/programmer. Board is configured as follows:

- UART4 TX/RX : PD1/PD0 (ST-Link Virtual Port Com)
- LED1 (red) : PO1
- LED2 (blue) : PO5
- USER PUSH-BUTTON : PC13

System Clock
------------

ART-Pi2 System Clock could be driven by an internal or external
oscillator, as well as the main PLL clock. By default, the System clock is
driven by the PLL clock at 250MHz, driven by an 24MHz high-speed external clock.

Serial Port
-----------

ART-Pi2 board has 4 UARTs and 3 USARTs plus one LowPower UART. The Zephyr console
output is assigned to UART4. Default settings are 115200 8N1.

Backup SRAM
-----------

In order to test backup SRAM you may want to disconnect VBAT from VDD. You can
do it by removing ``SB13`` jumper on the back side of the board.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

ART-Pi2 board includes an ST-LINK/V2.1 embedded debug tool interface.

.. note::

   Check if your ST-LINK V2.1 has newest FW version. It can be done with `STM32CubeProgrammer`_

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Flashing an application to ART-Pi2
----------------------------------

First, connect the art_pi2 to your host computer using
the USB port to prepare it for flashing. Then build and flash your application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your art_pi2 board.

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyACM0

or use screen:

.. code-block:: console

   $ screen /dev/ttyACM0 115200

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: art_pi2
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-1907-g415ab379a8af ***
   Hello World! art_pi2/stm32h7r7xx

Blinky example can also be used:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: art_pi2
   :goals: build flash

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: art_pi2
   :maybe-skip-config:
   :goals: debug

References
**********
.. target-notes::

.. _ART-Pi2 website:
   https://github.com/RT-Thread-Studio/sdk-bsp-stm32h7r-realthread-artpi2

.. _STM32H7R7L8 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h7r7l8.html

.. _STM32H7Rx reference manual:
   https://www.st.com/resource/en/reference_manual/rm0477-stm32h7rx7sx-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
