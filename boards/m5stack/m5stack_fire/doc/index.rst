.. zephyr:board:: m5stack_fire

Overview
********

M5Stack Fire is an ESP32-based development board from M5Stack.

M5Stack Fire features the following integrated components:

- ESP32-D0WDQ6 chip (240MHz dual core, 600 DMIPS, 520KB SRAM, Wi-Fi)
- PSRAM 8MB
- Flash 16MB
- LCD IPS TFT 2", 320x240 px screen (ILI9342C)
- Charger IP5306
- Audio NS4148 amplifier (1W-092 speaker)
- USB CH9102F
- SD-Card slot
- Grove connector
- IMO 6-axis IMU MPU6886
- MIC BSE3729
- Battery 500mAh 3,7V
- Three physical buttons
- LED strips

Functional Description
**********************

The following table below describes the key components, interfaces, and controls
of the M5Stack Core2 board.

.. _M5Core2 Schematic: https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/480/M5-Core-Schematic_20171206.pdf
.. _MPU-ESP32: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/esp32_datasheet_en_v3.9.pdf
.. _MPU-6886: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/MPU-6886-000193%2Bv1.1_GHIC_en.pdf
.. _LCD-ILI9342C: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/ILI9342C-ILITEK.pdf
.. _IP5306: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/IIC_IP5306_REG_V1.4_cn.pdf

+------------------+------------------------------------------------------------------------+-----------+
| Key Component    | Description                                                            | Status    |
+==================+========================================================================+===========+
| ESP32-D0WDQ6-V2  | This `MPU-ESP32`_ module provides complete Wi-Fi and Bluetooth         | supported |
| module           | functionalities and integrates a 16-MB SPI flash.                      |           |
+------------------+------------------------------------------------------------------------+-----------+
| USB Port         | USB interface. Power supply for the board as well as the               | supported |
|                  | communication interface between a computer and the board.              |           |
+------------------+------------------------------------------------------------------------+-----------+
| Power Switch     | Power on/off button.                                                   | supported |
+------------------+------------------------------------------------------------------------+-----------+
| General purpose  | Three buttons on the front face of the device accessible using the     | supported |
| buttons          | GPIO interface.                                                        |           |
+------------------+------------------------------------------------------------------------+-----------+
| LCD screen       | Built-in LCD TFT display \(`LCD-ILI9342C`_, 2", 320x240 px\)           | supported |
|                  | controlled via SPI interface                                           |           |
+------------------+------------------------------------------------------------------------+-----------+
| SD-Card slot     | SD-Card connection via SPI-mode.                                       | supported |
+------------------+------------------------------------------------------------------------+-----------+
| 6-axis IMU       | The `MPU-6886`_ is a 6-axis motion tracker (6DOF IMU) device that      | supported |
| MPU6886          | combines a 3-axis gyroscope and a 3-axis accelerometer.                |           |
+------------------+------------------------------------------------------------------------+-----------+
| Grove port       | Used to interface with the many available modules and sensors.         | supported |
+------------------+------------------------------------------------------------------------+-----------+
| Built-in speaker | 1W speaker for analog audio output.                                    | supported |
+------------------+------------------------------------------------------------------------+-----------+
| Built-in         | The BSE3729 analog microphone.                                         | todo      |
| microphone       |                                                                        |           |
+------------------+------------------------------------------------------------------------+-----------+
| LED strip        | LED strips on the side of the device.                                  | todo      |
+------------------+------------------------------------------------------------------------+-----------+
| Battery-support  | Charging is supported automatically via the `IP5306`_. But there is no | todo      |
|                  | possibility to query current battery status.                           |           |
+------------------+------------------------------------------------------------------------+-----------+

Supported Features
==================

.. zephyr:board-supported-hw::

Start Application Development
*****************************

Before powering up your M5Stack Fire, please make sure that the board is in good
condition with no obvious signs of damage.

System requirements
===================

Prerequisites
-------------

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building & Flashing
-------------------

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: m5stack_fire/esp32/procpu
   :goals: build

The usual ``flash`` target will work with the ``m5stack_fire`` board
configuration. Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: m5stack_fire/esp32/procpu
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
   Hello World! m5stack_fire

Debugging
---------

M5Stack Fire debugging is not supported due to pinout limitations.

Related Documents
*****************

- `M5Stack-Fire schematic <https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/480/M5-Core-Schematic_20171206.pdf>`_ (PDF)
- `M5Stack-Fire docs <https://docs.m5stack.com/en/core/fire_v2.7>`_
- `ESP32 Datasheet <https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf>`_ (PDF)
- `ESP32 Hardware Reference <https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/index.html>`_
