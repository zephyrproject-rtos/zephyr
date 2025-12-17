.. zephyr:board:: rtl8721f_evb

Overview
********

The Realtek RTL8721F Series is a low-power, single-chip microcontroller featuring dual RISC cores
compatible with the Arm® Cortex®-M55 instruction set, designed for optimized power efficiency,
RF performance, and ultra-low power consumption. It incorporates comprehensive low-power features
such as fine-grained clock gating, multiple power modes, and dynamic power scaling. As a dual-band
(2.4 GHz and 5 GHz) communication controller, the RTL8721F integrates Wi-Fi 6
(802.11a/b/g/n/ac/ax with 20 MHz bandwidth) and Bluetooth Core 5.4 Qualified specifications,
combining a WLAN MAC, a 1T1R-capable WLAN baseband and RF, along with Bluetooth functionality to
deliver complete wireless connectivity solutions. For more information, check `RTL8721F-EVB`_.

The features include the following:

- Dual cores: two Real-M300 (or KM4 thereafter) processors

  - KM4TZ: KM4 with Arm TrustZone®-M security technology, usually works as Application Processor
  - KM4NS: KM4 without Arm TrustZone®-M security technology, usually works as Network Processor

- 512KB on-chip SRAM
- Wi-Fi 6 (802.11 a/b/g/n/ac/ax), 1T1R, 2.4GHz/5GHz dual-band
- Bluetooth 5.0 LE
- Peripheral Interface:

  - Flexible design of GPIO configuration
  - Multi-communication interfaces: SPI x 2, UART x 5, I2C x 2, I2C-like x 1
  - Hardware IR transceiver, easy to adapt to various IR protocols
  - Supports Real-Time Clock timer together with 4 basic timers
  - Supports 4 x 4 channels of PWM timer and 1 capture timer
  - Supports 2 group PMC timers
  - Supports a general 12-bit ADC: 8 external channels and 4 internal channels
  - Supports 9 channels of touch pad
  - Supports 8 independent channels of GDMA
  - Supports USB 2.0 High-speed in either Host or Device mode
  - Supports SDIO in both Host and Device modes, with options for 1-bit and 4-bit modes
  - Supports SD host controller to access SD card and eMMC device
  - Integrated Pixel Processing Engine (PPE) to process pixel data faster
  - LCDC
  - MJPEG hardware decoder to enhance HMI performance
  - A2C (compatible with ISO 11898-1, CAN Specification 2.0)
  - Ethernet MAC
  - Integrated audio codec supports 2 channels DMIC interface
  - I2S x 1: up to 384kHz sampling rate
  - Supports thermal detector to detect and monitor the real-time temperature inside the chip

- Cryptographic hardware acceleration (TRNG, ECC/RSA, SHA-2, AES，Flash XIP decryption)

For more information, Get application note and datasheet at `RTL8721F Series`_ depending on chip you use.

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
   :board: rtl8721f_evb
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

Using SWD through PA18(SWD_CLK) and PA19(SWD_DAT).

References
**********

.. _`RTL8721F-EVB`: https://www.realmcu.com/en/Home/Product/add965ea-d661-4a63-9514-d18b6912f8ab#
.. _`RTL8721F Series`: https://www.realmcu.com
.. _`AmebaImageTool`: https://github.com/Ameba-AIoT/ameba-rtos/blob/master/tools/ameba/ImageTool/AmebaImageTool.exe
