.. zephyr:code-sample:: esp32s3_box3_demo
   :name: ESP32-S3-BOX-3 Demo
   :relevant-api: gpio_interface i2c_interface display_interface

   Demonstrate ESP32-S3-BOX-3 board features including GPIO, I2C, and display.

Overview
********

This sample demonstrates the basic functionality of the ESP32-S3-BOX-3 board:

* GPIO input (boot button)
* I2C interface (sensor communication)
* Display interface (ILI9341 LCD)
* Serial console output

The application will:

1. Test GPIO by reading the boot button state
2. Scan the I2C bus for connected devices (like the AHT30 sensor)
3. Initialize and test the display interface
4. Continuously monitor the boot button state

Building and Running
********************

This application can be built and executed on the ESP32-S3-BOX-3 board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/esp32s3_box3
   :board: esp32s3_box3/esp32s3/procpu
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   [00:00:00.123,000] <inf> esp32s3_box3_demo: ESP32-S3-BOX-3 Demo Application Starting...
   [00:00:00.124,000] <inf> esp32s3_box3_demo: Board: esp32s3_box3/esp32s3/procpu
   [00:00:00.125,000] <inf> esp32s3_box3_demo: SoC: esp32s3
   [00:00:00.126,000] <inf> esp32s3_box3_demo: Testing GPIO (Boot Button)...
   [00:00:00.127,000] <inf> esp32s3_box3_demo: Boot button state: RELEASED
   [00:00:00.128,000] <inf> esp32s3_box3_demo: Testing I2C interface...
   [00:00:00.129,000] <inf> esp32s3_box3_demo: I2C device ready - scanning for devices...
   [00:00:00.135,000] <inf> esp32s3_box3_demo: Found I2C device at address 0x38
   [00:00:00.140,000] <inf> esp32s3_box3_demo: Testing display interface...
   [00:00:00.141,000] <inf> esp32s3_box3_demo: Display capabilities:
   [00:00:00.142,000] <inf> esp32s3_box3_demo:   Resolution: 320x240
   [00:00:00.143,000] <inf> esp32s3_box3_demo:   Supported pixel formats: 0x00000001
   [00:00:00.144,000] <inf> esp32s3_box3_demo:   Current pixel format: 0
   [00:00:00.145,000] <inf> esp32s3_box3_demo:   Current orientation: 0
   [00:00:00.146,000] <inf> esp32s3_box3_demo: Display blanking turned off
   [00:00:00.147,000] <inf> esp32s3_box3_demo: Demo completed. System running...
   [00:00:05.148,000] <inf> esp32s3_box3_demo: Boot button: RELEASED