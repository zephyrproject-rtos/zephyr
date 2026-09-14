.. zephyr:board:: pke8721daf_c13_f10

Overview
********

The PKE8721DAF-C13-F10 is an evaluation board based on the Realtek RTL8721DAF-VT2-CG, a combo SoC
that supports dual-band Wi-Fi 4 (2.4GHz + 5GHz) and BLE 5.0. With its ultra-low power consumption
and abundant peripheral resources, it is widely used in smart home appliances, line controllers,
smart door locks, battery cameras, smart remote controllers, Wi-Fi speakers, BLE gateways, and
smart POS, etc. For more information, check `PKE8721DAF-C13-F10`_.

Hardware
********

The features of the RTL8721DAF include the following:

- Dual cores:

  - KM4 running at up to 345MHz
  - KM0 running at up to 115MHz

- 4MB MCM flash
- 512KB on-chip SRAM
- Wi-Fi:

  - 802.11 a/b/g/n 1 x 1, 2.4GHz + 5GHz
  - Up to 40MHz bandwidth

- Bluetooth:

  - BLE 5.0
  - Supports LE-1M / LE-2M / LE-Coded PHY (long range)

- Peripheral interfaces:

  - UART x 4 (up to 8Mbps)
  - I2C x 2 (up to 3.4Mbps in high-speed mode)
  - SPI x 2 (up to 50Mbps)
  - 8 channels of PWM
  - (7 + 1) channels of 12-bit ADC
  - 4 channels of Cap-Touch detection
  - USB 2.0 full-speed device interface x 1
  - SDIO device interface x 1 (up to 50MHz in high-speed mode)
  - IR interface x 1

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
   :board: pke8721daf_c13_f10
   :goals: build

Flashing
========

The standard ``flash`` target is supported by the board configuration.

**Enter Download Mode**

To flash the device, you must first put the SoC into download mode using the on-board
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
   :board: pke8721daf_c13_f10
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
   :board: pke8721daf_c13_f10
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _`PKE8721DAF-C13-F10`: https://aiot.realmcu.com/zh/center/hardware/detail/56?theme_id=5
