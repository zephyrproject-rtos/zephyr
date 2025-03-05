.. zephyr:board:: m5stack_atoms3_lite

Overview
********

M5Stack AtomS3 Lite is an ESP32-based development board from M5Stack.

It features the following integrated components:

- ESP32-S3FN8 chip (240MHz dual core, Wi-Fi/BLE 5.0)
- 512KB of SRAM
- 384KB of ROM
- 8MB of Flash
- RGB Status-LED

Supported Features
==================

.. zephyr:board-supported-hw::

Start Application Development
*****************************

Before powering up your M5Stack AtomS3 Lite, please make sure that the board is in good
condition with no obvious signs of damage.

System requirements
===================

Prerequisites
-------------

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: shell

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building & Flashing
-------------------

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: m5stack_atoms3_lite/esp32s3/procpu
   :goals: build

The usual ``flash`` target will work with the ``m5stack_atoms3_lite`` board
configuration. Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: m5stack_atoms3_lite/esp32s3/procpu
   :goals: flash

The baud rate of 921600bps is set by default. If experiencing issues when flashing,
try using different values by using ``--esp-baud-rate <BAUD>`` option during
``west flash`` (e.g. ``west flash --esp-baud-rate 115200``).

You can also open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! m5stack_atoms3_lite

Debugging
---------

M5Stack AtomS3 Lite debugging is not supported due to pinout limitations.

Related Documents
*****************

- `M5Stack AtomS3 Lite schematic <https://static-cdn.m5stack.com/resource/docs/products/core/AtomS3%20Lite/img-4061fdd4-6954-4709-a7e7-b0f50e5ba52e.webp>`_
- `ESP32S3 Datasheet <https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf>`_
