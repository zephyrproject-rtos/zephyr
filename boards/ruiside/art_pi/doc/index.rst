.. zephyr:board:: art_pi

Overview
********

The ART-Pi is an open-source hardware platform designed by the
RT-Thread team specifically for embedded software engineers
and open-source makers, offering extensive expandability for DIY projects.

Key Features

- STM32H750XBH6 microcontroller featuring 128 Kbytes of Flash and 1024 Kbytes of SRAM in an TFBGA225 package
- On-board ST-LINK/V2.1 debugger/programmer
- SDIO TF Card slot
- SDIO WIFI:AP6212
- HDC UART BuleTooth:AP6212
- 32-MB SDRAM
- 16-Mbytes SPI FLASH
- 8-Mbytes QSPI FLASH
- One Power LED (blue) for 3.3 V power-on
- Two user LEDs blue and red
- Two ST-LINK LEDs: blue and red
- Two push-buttons (user and reset)
- Board connectors:

  - USB OTG with Type-C connector
  - RGB888 FPC connector

More information about the board can be found at the `ART-Pi website`_.

Hardware
********

ART-Pi provides the following hardware components:

The STM32H750xx devices are a high-performance microcontrollers family (STM32H7
Series) based on the high-performance Arm |reg| Cortex |reg|-M7 32-bit RISC core.
They operate at a frequency of up to 480 MHz.

More information about STM32H750xx can be found here:

- `STM32H750 on www.st.com`_

Supported Features
==================

.. zephyr:board-supported-hw::

Default Zephyr Peripheral Mapping:
----------------------------------

The ART-Pi board features a On-board ST-LINK/V2.1 debugger/programmer. Board is configured as follows:

- UART4 TX/RX : PA0/PI9 (ST-Link Virtual Port Com)
- LED1 (red) : PC15
- LED2 (blue) : PI8
- USER PUSH-BUTTON : PH4

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. note::

   Check if your ST-LINK V2.1 has newest FW version. It can be done with `STM32CubeProgrammer`_

Flashing
========

First, connect the art_pi to your host computer using
the USB port to prepare it for flashing. Then build and flash your application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your art_pi board.

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyACM0

or use screen:

.. code-block:: console

   $ screen /dev/ttyACM0 115200

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: art_pi
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-3809-g1d6b6759aa1a ***
   Hello World! art_pi/stm32h750xx

Blinky example can also be used:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: art_pi
   :goals: build flash

To flash an application that requires loading firmware on
external flash, see `ART_PI Externloader`_

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: art_pi
   :maybe-skip-config:
   :goals: debug

References
**********
.. target-notes::

.. _ART-Pi website:
   https://github.com/RT-Thread-Studio/sdk-bsp-stm32h750-realthread-artpi

.. _STM32H750 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h750xb.html

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _ART_PI Externloader:
   https://github.com/newbie-jiang/art_pi_use_externalLoader_doc
