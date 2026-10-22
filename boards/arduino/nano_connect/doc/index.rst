.. zephyr:board:: arduino_nano_connect

Overview
********

The Arduino Nano RP2040 Connect is a powerful addition to the Arduino ecosystem that brings the
RP2040 microcontroller together with Wi-Fi and Bluetooth connectivity. Whether you're a beginner
stepping into the world of IoT or an advanced user looking to incorporate it into your next
product, the Nano RP2040 Connect is a versatile choice.

Key features of the Nano RP2040 Connect:

- Tiny footprint: Designed with the well-known Nano form factor in mind, this board's compact size
  makes it perfect for embedding in standalone projects.
- Wi-Fi and Bluetooth: The u-blox NINA-W102 module provides Wi-Fi 802.11b/g/n and Bluetooth 4.2
  connectivity, making it ideal for IoT applications.
- Onboard sensors: Features an LSM6DSOX 6-axis IMU (accelerometer and gyroscope) and an MP34DT06J
  digital microphone for audio and motion sensing applications.
- Security: Includes an ATECC608A cryptographic coprocessor for secure IoT applications.
- RGB LED: An onboard RGB LED driven by the NINA module for visual feedback.

Hardware
********

- Raspberry Pi RP2040 (dual-core Arm Cortex-M0+ at 133 MHz)
- SRAM: 264 KB
- Flash: 16 MB (AT25SF128A)
- u-blox NINA-W102 (Wi-Fi 802.11b/g/n, Bluetooth 4.2)
- ST LSM6DSOX 6-axis IMU
- ST MP34DT06J MEMS Microphone
- Microchip ATECC608A Crypto
- Built-in LED on GPIO6
- Digital I/O Pins: 22
- PWM Pins: 20
- Analog Input Pins: 8
- Operating Voltage: 3.3V
- Input Voltage (VIN): 5-21V

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `Arduino Store`_ and `Arduino Docs`_ have detailed information about board
connections. Download the `datasheet`_ for more detail.

Flashing
========

Using UF2
---------

The easiest way to flash the board is using the UF2 bootloader.
Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: arduino_nano_connect
   :goals: build flash

Serial Console
==============

Connect to the serial console using the USB port:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

You should see the following message:

.. code-block:: console

   Hello World! arduino_nano_connect

Debugging
=========

You can debug an application using OpenOCD and a debug probe. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: arduino_nano_connect
   :goals: debug

References
**********

.. target-notes::

.. _Arduino Store:
    https://store.arduino.cc/products/arduino-nano-rp2040-connect

.. _Arduino Docs:
    https://docs.arduino.cc/hardware/nano-rp2040-connect/

.. _datasheet:
    https://docs.arduino.cc/resources/datasheets/ABX00053-datasheet.pdf
