.. zephyr:board:: rtl872xd_evb

Overview
********

The Realtek RTL872xD Series is a Combo SoC that supports dual-band Wi-Fi 4 (2.4GHz + 5GHz) and
BLE 5.0 specifications. With ultra-low power consumption, complete encryption strategy and abundant
peripheral resources, it is widely in various products such as Home appliance control panel,
Smart door, Smart toy, Smart voice, Smart remote control, Bluetooth gateway, Headset, Wi-Fi gamepad,
Smart POS, etc. For more information, check `RTL872XD-EVB`_.

The features include the following:

- Dual cores: Real-M300 and Real-M200
- 512KB + 64KB on-chip SRAM
- 802.11 a/b/g/n 1 x 1, 2.4GHz + 5GHz
- Supports BLE 5.0
- Peripheral Interface:

  - Multi-communication interfaces: SPI x 2, UART x 4, I2C x 1
  - Hardware Key-scan interface supports up to 36 keys
  - Hardware Quad-decoder supports statistical and comparison functions
  - Hardware IR transceiver can easily adapt to various IR protocols
  - SDIO/USB high speed interface (both host and slave)
  - Supports real-time clock together with 18 channels of PWM output
  - Supports 5 channels of touch pad and 6 channels of GDMA
  - Supports 7 channels of normal 12-bit ADC and 1 channel of VBAT
  - Integrated LCDC supports both RGB and I8080 interfaces
  - Integrated hardware crypto engine supports AES256/192/128 and SHA256
  - Integrated audio codec

For more information, Get application note and datasheet at `RTL872xCS/D Series`_ depending on chip you use.

Supported Features
==================

.. zephyr:board-supported-hw::

Prerequisites
*************

Realtek HAL requires binary blobs in order work. Run the command below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_realtek

.. note::

   It is recommended running the command above after ``west update``.

Building
********

Here is an example for building the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rtl872xd_evb
   :goals: build

Flashing
********

When the build finishes, downloading images into flash by `AmebaImageTool`_:

See the ApplicationNote chapter Image Tool from documentation links for more details.

#. Environment Requirements: EX. WinXP, Win 7 or later, Microsoft .NET Framework 4.0.
#. Connect chip and PC with USB wire.
#. Choose the Device profiles according to the chip you use.
#. Select the corresponding serial port and transmission baud rate. The default baud rate is 1500000.
#. Select the images to be programmed and set the start address and end address according to the flash layout, refer to [ameba_flashcfg.c/Flash_layout].
#. Click the Download button and start. The progress bar will show the download progress of each image and the log window will show the operation status.

.. note::

   For an empty chip, the bootloader and app image shall be downloaded.

Debugging
*********

Using SWD through PB3(SWD_CLK) and PA27(SWD_DAT).

References
**********

.. _`RTL872XD-EVB`: https://www.realmcu.com/en/Home/Products/RTL872xCS-RTL872xD-Series#
.. _`RTL872xCS/D Series`: https://www.realmcu.com
.. _`AmebaImageTool`: https://github.com/Ameba-AIoT/ameba-rtos/tree/master/tools/ameba/ImageTool_Legacy/AmebaImageTool.exe
