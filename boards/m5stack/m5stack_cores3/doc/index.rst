.. zephyr:board:: m5stack_cores3

Overview
********

M5Stack CoreS3 is an ESP32-based development board from M5Stack. It is the third generation of the M5Stack Core series.

M5Stack CoreS3 features consist of:

- ESP32-S3 chip (dual-core Xtensa LX7 processor @240MHz, WIFI, OTG and CDC functions)
- PSRAM 8MB
- Flash 16MB
- LCD ISP 2", 320x240 pixel ILI9342C
- Capacitive multi touch FT6336U
- Camera 30W pixel GC0308
- Speaker 1W AW88298
- Dual Microphones ES7210 Audio decoder
- RTC BM8563
- USB-C
- SD-Card slot
- Geomagnetic sensor BMM150
- Proximity sensor LTR-553ALS-WA
- 6-Axis IMU BMI270
- PMIC AXP2101
- Battery 500mAh 3.7 V

Start Application Development
*****************************

Before powering up your M5Stack CoreS3, please make sure that the board is in good
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

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: m5stack_cores3/esp32s3/procpu
   :goals: build

The usual ``flash`` target will work with the ``m5stack_cores3/esp32s3/procpu`` board
configuration. Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: m5stack_cores3/esp32s3/procpu
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

   *** Booting Zephyr OS build vx.x.x-xxx-gxxxxxxxxxxxx ***
   Hello World! m5stack_cores3/esp32s3/procpu


Debugging
---------

ESP32-S3 support on OpenOCD is available at `OpenOCD ESP32`_.

ESP32-S3 has a built-in JTAG circuitry and can be debugged without any additional chip. Only an USB cable connected to the D+/D- pins is necessary.

Further documentation can be obtained from the SoC vendor in `JTAG debugging for ESP32-S3`_.

References
**********

.. target-notes::

.. _`M5Stack CoreS3 Documentation`: http://docs.m5stack.com/en/core/CoreS3
.. _`M5Stack CoreS3 Schematic`: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/K128%20CoreS3/Sch_M5_CoreS3_v1.0.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
.. _`JTAG debugging for ESP32-S3`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/
