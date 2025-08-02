.. zephyr:board:: ttgo_t3stm32

Overview
********

The Lilygo TTGO T3-STM32 is a development board for LoRa applications based on the
STMicroelectronics STM32WL55 ARM Cortex-M4/Cortex-M0+ dual core MCU.

It features the following integrated components:

- STM32WL55 MCU (48MHz dual core M4/M0+, 256kB flash, 64kB RAM, LPWAN RF)
- SSD1315, 128x64 px, 0.96" OLED screen
- SD card
- JST PH 2-pin battery connector
- two LEDs

Some of the MCUs I/O pins are accessible on the board's pin headers.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Start Application Development
*****************************

Before powering up your Lilygo TTGO T3-STM32, please make sure that the board is in good
condition with no obvious signs of damage.

Flashing
********

Flashing requires the `stm32flash tool`_.

Building stm32flash command line tool
=====================================

To build the stm32flash tool, follow the steps below:

#. Checkout the stm32flash tool's code from the repository.

  .. code-block:: console

     $ git clone http://git.code.sf.net/p/stm32flash/code stm32flash
     $ cd stm32flash

#. Build the stm32flash tool.

  .. code-block:: console

     $ make

#. The resulting binary is available at :file:`stm32flash`.

Flashing an Application
=======================

To upload an application to the Lilygo TTGO T3-STM32, connect the board via the
USB Type-C connector, the device will enumerate as a virtual com port.
This tutorial uses the :zephyr:code-sample:`hello_world` sample application.

#. Press the reset button while holding the boot button.

#. To build the application and flash it, enter:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: t3_stm32
      :goals: flash

#. Run your favorite terminal program to listen for output.

   .. code-block:: console

      $ minicom -D /dev/ttyACM0 -b 115200

   The :code:`-b` option sets baud rate ignoring the value
   from config.

#. Press the Reset button and you should see the output of
   the hello world application in your terminal.

.. note::
   Make sure your terminal program is closed before flashing
   the binary image, or it will interfere with the flashing
   process.

Code samples
============

The following sample applications will work out of the box with this board:

* :zephyr:code-sample:`hello_world`
* :zephyr:code-sample:`blinky`
* :zephyr:code-sample:`lora-send`
* :zephyr:code-sample:`lora-receive`
* :zephyr-app: samples/subsys/fs/fs_sample

Related Documents
*****************
- `Lilygo TTGO T3STM32 schematic <https://github.com/Xinyuan-LilyGO/T3-STM32/blob/master/hardware/T3_STM32 V1.0%2024-07-30.pdf>`_ (PDF)
- `Lilygo TTGO T3STM32 documentation <https://github.com/Xinyuan-LilyGO/T3-STM32>`_
- `Lilygo github repo <https://github.com/Xinyuan-LilyGo>`_
- `STM32WL55JC datasheet <https://www.st.com/resource/en/datasheet/stm32wl55jc.pdf>`_ (PDF)
- `STM32WL55JC MCU documentation <https://www.st.com/en/microcontrollers-microprocessors/stm32wl55jc.html>`_
- `stm32flash tool <https://sourceforge.net/p/stm32flash/wiki/Home>`_
- `SSD1315 datasheet <https://github.com/Xinyuan-LilyGO/T3-STM32/blob/master/hardware/SSD1315.pdf>`_ (PDF)
