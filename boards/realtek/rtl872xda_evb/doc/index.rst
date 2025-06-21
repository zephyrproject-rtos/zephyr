.. zephyr:board:: rtl872xda_evb

Overview
********

The Realtek RTL8721Dx Series is a Combo SoC that supports dual-band Wi-Fi 4 (2.4GHz + 5GHz) and
BLE 5.0 specifications. With excellent ultra-low power consumption, enhanced encryption strategy
(PSA Level 2), and abundant peripheral resources, it is widely used in smart home appliance,
line controller, smart door lock, battery camera, smart remote controller, Wi-Fi Speaker, Wi-Fi
Full MAC NIC, BLE gateway, and smart POS, etc. For more information, check `RTL872XDA-EVB`_.

The features include the following:

- Dual cores: Real-M300 and Real-M200
- 512KB on-chip SRAM
- 802.11 a/b/g/n 1 x 1, 2.4GHz + 5GHz
- Supports BLE 5.0
- Peripheral Interface:

  - Multi-communication interfaces: SPI x 2, UART x 4, I2C x 2
  - Hardware Key-Scan interface supports up to 8*8 (64) keys
  - Hardware IR transceiver can easily adapt to various IR protocols
  - Supports Real-Time Clock timer together with 10 basic timers
  - Supports 8 channels of PWM timer and 1 capture timer
  - Supports 7 channels of general 12-bit ADC and 1 channel of VBAT
  - Supports 4 channels of touch pad
  - Supports 8 independent channels of GDMA
  - Supports USB 2.0 full-speed device mode
  - Supports SDIO device with 1-bit and 4-bit mode
  - Embeds a serial LEDC to control the external LED lamps
  - Integrated Pixel Processing Engine (PPE) to process pixel data faster
  - Integrated OSPI display interface supports screens with OSPI/QSPI/SPI interfaces
  - Integrated audio codec supports 2 channels DMIC interface
  - I2S x 2: up to 384kHz sampling rate

- Cryptographic hardware acceleration (TRNG, ECC, SHA-2, AES)

For more information, Get application note and datasheet at `RTL8721Dx Series`_ depending on chip you use.

Supported Features
==================

.. zephyr:board-supported-hw::

Building
********

Here is an example for building the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rtl872xda_evb
   :goals: buil

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

Using SWD through PA30(SWD_CLK) and PA31(SWD_DAT).

References
**********

.. _`RTL872XDA-EVB`: https://www.realmcu.com/en/Home/Product/add965ea-d661-4a63-9514-d18b6912f8ab#
.. _`RTL8721Dx Series`: https://www.realmcu.com
.. _`AmebaImageTool`: https://github.com/Ameba-AIoT/ameba-rtos/blob/master/tools/ameba/ImageTool/AmebaImageTool.exe
