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

Hardware
********

The features of RTL8721F include the following:

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

For more information, refer to the `RTL8721F Application Note`_, the `RTL8721F Datasheet`_, and
the `RTL8721F Pinmux Table`_, depending on the chip you are using.

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

Binary Blobs
============

Realtek HAL requires binary blobs in order work. Run the command below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_realtek

.. note::

   It is recommended running the command above after ``west update``.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

An example of building the :zephyr:code-sample:`hello_world` application is shown below:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rtl8721f_evb
   :goals: build

Flashing
========

The standard ``flash`` target is supported by the board configuration.

**Enter Download Mode**

To flash the device, you must first put the SoC into download mode using the on-board FT232RL
USB-to-UART converter. Follow these steps:

1. USB Connection: Connect a USB cable to the ``USB2UART`` port.
2. Enter Download Mode:

   - Press and hold the ``Download`` button.
   - While holding it, briefly press the ``Chip_En`` button to reset the SoC.
   - Release the ``Download`` button after the reset.

The SoC is now in download mode and ready to receive firmware.

.. note::

   The ``<port_name>`` placeholder in the flash command must be replaced with the actual serial port
   (e.g., ``/dev/ttyUSB0`` on Linux or ``COM3`` on Windows).

An example for flashing the :zephyr:code-sample:`hello_world` application is shown below:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rtl8721f_evb
   :goals: flash
   :flash-args: --port <port_name>

**Enter Normal Mode**

After flashing completes successfully, press the ``Chip_En`` button once to reboot the SoC.
The device then boots into normal mode and executes the flashed Zephyr firmware.

Debugging
=========

Debugging is supported via SWD using a J-Link debug probe connected to PA18 (SWD_CLK) and PA19 (SWD_DAT).
An example debug command for the :zephyr:code-sample:`hello_world` application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rtl8721f_evb
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _`RTL8721F-EVB`: https://aiot.realmcu.com/en/home.html
.. _`RTL8721F Application Note`: https://aiot.realmcu.com/cn/latest/
.. _`RTL8721F Datasheet`: https://aiot.realmcu.com/en/datasheet/index.html
.. _`RTL8721F Pinmux Table`: https://aiot.realmcu.com/en/pinmux/pinmux_table.html
