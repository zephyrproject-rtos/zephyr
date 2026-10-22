.. zephyr:board:: rtl872xda_evb

Overview
********

The Realtek RTL8721Dx Series is a Combo SoC that supports dual-band Wi-Fi 4 (2.4GHz + 5GHz) and
BLE 5.0 specifications. With excellent ultra-low power consumption, enhanced encryption strategy
(PSA Level 2), and abundant peripheral resources, it is widely used in smart home appliance,
line controller, smart door lock, battery camera, smart remote controller, Wi-Fi Speaker, Wi-Fi
Full MAC NIC, BLE gateway, and smart POS, etc. For more information, check `RTL872XDA-EVB`_.

Hardware
********

The features of RTL8721Dx include the following:

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

For more information, refer to the `RTL8721Dx Application Note`_, the `RTL8721Dx Datasheet`_, and
the `RTL8721Dx Pinmux Table`_, depending on the chip you are using.

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
   :board: rtl872xda_evb
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
   :board: rtl872xda_evb
   :goals: flash
   :flash-args: --port <port_name>

**Enter Normal Mode**

After flashing completes successfully, press the ``Chip_En`` button once to reboot the SoC.
The device then boots into normal mode and executes the flashed Zephyr firmware.

Debugging
=========

Debugging is supported via SWD using a J-Link debug probe connected to PA30 (SWD_CLK) and PA31 (SWD_DAT).
An example debug command for the :zephyr:code-sample:`hello_world` application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rtl872xda_evb
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _`RTL872XDA-EVB`: https://aiot.realmcu.com/en/product/rtl8721dx.html
.. _`RTL8721Dx Application Note`: https://aiot.realmcu.com/cn/latest/
.. _`RTL8721Dx Datasheet`: https://aiot.realmcu.com/en/datasheet/index.html
.. _`RTL8721Dx Pinmux Table`: https://aiot.realmcu.com/en/pinmux/pinmux_table.html
