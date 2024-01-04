.. _m5stack_m5dial:

M5Stack M5Dial
##############

Overview
********

M5Dial is an ESP32-based development board from M5Stack.
The device features an embedded ``m5stack_stamps3`` module that runs
the ESP32-S3 microcontroller.

.. figure:: img/m5stack_m5dial.webp
        :align: center
        :alt: M5Stack M5Dial

        M5Stack M5Dial

Key components
==============

The Zephyr ``m5stack_m5dial`` shield configuration supports the following key components:

+------------------------+-----------------------------------------------------------------------+------------+
| Key Component          | Description                                                           | Status     |
+========================+=======================================================================+============+
| LCD IPS round display  | 240x240 pixel display features GC9A01A display controller interfaced  | supported  |
| (240x240 pixel)        | via SPI                                                               |            |
+------------------------+-----------------------------------------------------------------------+------------+
| WS1850S NFC            | Wisesun NFC reader IC interfaced via I2C.                             | todo       |
+------------------------+-----------------------------------------------------------------------+------------+
| RTC8563 RTC            | Real-Time clock interfaced via I2C.                                   | supported  |
+------------------------+-----------------------------------------------------------------------+------------+
| Rotary encoder         | Round rotary button (``INPUT_KEY_LEFT`` and ``INPUT_KEY_RIGHT``       | supported  |
|                        | events).                                                              |            |
+------------------------+-----------------------------------------------------------------------+------------+
| Reset button           | Directly interconnected to EN signal of StampS3 module.               | supported  |
+------------------------+-----------------------------------------------------------------------+------------+
| User-button            | Custom user button.                                                   | supported  |
+------------------------+-----------------------------------------------------------------------+------------+
| Builtin Beeper/Buzzer  | PWM controller beeper, properly operating between 200Hz and 2kHz      | supported  |
+------------------------+-----------------------------------------------------------------------+------------+

M5Stack M5Dial features 2 external grove ports (A and B)

Grove header A
---------------

Functionality is accessible via ``grove_headerA``

+-----+----------------------------+---------------------------+
| Pin | ``m5stack_stamps3_header`` | Function                  |
+=====+============================+===========================+
| 1   | 16                         | I2C-SCL (``groveA_i2c```) |
+-----+----------------------------+---------------------------+
| 2   | 14                         | I2C-SDL (``groveA_i2c``)  |
+-----+----------------------------+---------------------------+
| 3   |                            | 5V-Output                 |
+-----+----------------------------+---------------------------+
| 4   |                            | GND                       |
+-----+----------------------------+---------------------------+


Grove header B
---------------

Functionality is accessible via ``grove_headerB``

+-----+----------------------------+-----------+
| Pin | ``m5stack_stamps3_header`` | Function  |
+=====+============================+===========+
| 1   | 0                          | In        |
+-----+----------------------------+-----------+
| 2   | 1                          | Out       |
+-----+----------------------------+-----------+
| 3   |                            | 5V-Output |
+-----+----------------------------+-----------+
| 4   |                            | GND       |
+-----+----------------------------+-----------+


Battery-Support
---------------

Although ``m5stack_m5dial`` does not natively come with a battery, it features
support for connecting a battery to it. The battery connector is underneath the
StampS3 module.

To keep the module running on the battery only, without an external supply, an
internal power-hold signal must be activated. This can be controlled via regulator
``battery_hold``, which is activated by default.

Pin Mapping
===========

M5Dial is utilizing the :dtcompatible:`m5stack,stamps3-header` header.
Following table shows the pin mapping, the interconnection to the
M5Stack-StampS3 module and its according function.

+----------+--------------------------------+---------+-----------+-----------------------------------+
| StampS3  | stamps3-header                 | M5Dial  | M5Dial    | Description                       |
| pin      |                                | pin     | signal    |                                   |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-1     | ``m5stack_stamps3_header:0``   | J2.1    | GI        | Grove-B In                        |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-2     | ``m5stack_stamps3_header:1``   | J2-2    | GO        | Grove-B Out                       |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-3     | ``m5stack_stamps3_clkout0:2``  |         | Beep      | Beeper                            |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-4     | ``m5stack_stamps3_header:3``   |         | LCD_RS    | LCD Cmd/Data                      |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-5     | ``m5stack_stamps3_spilcd``     |         | LCD_MOSI  | LCD SPI-MOSI                      |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-6     | ``m5stack_stamps3_spilcd``     |         | LCD_SCK   | LCD SPI-CLK                       |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-7     | ``m5stack_stamps3_spilcd``     |         | LCD_CS    | LCD SPI-CS                        |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-8     | ``m5stack_stamps3_header:7``   |         | LCD_RESET | LCD Reset (active-low)            |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-9     | ``m5stack_stamps3_clkout0:0``  |         | LCD_BL    | LCD Backlight                     |
|          |                                |         |           | (PWM controlled MOSFET)           |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-10    | ``m5stack_stamps3_header:9``   |         | RC522_INT | NFC Interrupt                     |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-11    | **GND**                                              | Ground                            |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-12    | ``m5stack_stamps3_i2c1``       |         | TP_SDA    | Internal I2C-SDA                  |
|          |                                |         |           | (Touch, NFC and RTC)              |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-13    | **VIN_5V**                                           | 5V Input voltage                  |
|          |                                                      | (via USB-C connector of StampS3)  |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-14    | ``m5stack_stamps3_i2c1``       |         | TP_SCL    | Internal I2C-SCL                  |
|          |                                |         |           | (Touch, NFC and RTC)              |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-15    | ``m5stack_stamps3_i2c0``       | J3-2    | SDA       | Grove-A I2C-SDA                   |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-16    | ``m5stack_stamps3_header:15``  |         | TP_INT    | Touch Interrupt                   |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-17    | ``m5stack_stamps3_i2c0``       | J3-1    | SCL       | Grove-A I2C-SCL                   |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-18    | **GND**                                              | Ground                            |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-19    |                                |         | MTCK      |                                   |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-20    |                                |         | Boot      |                                   |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-21    | ``m5stack_stamps3_header:20``  |         | B         | Rotary left key                   |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-22    |                                |         | EN        | Enable signal for 3V3             |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-23    | ``m5stack_stamps3_header:22``  |         | A         | Rotary right key                  |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-24    |                                |         | RX        |                                   |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-25    | ``m5stack_stamps3_header:24``  |         | WAKE      | User-Button                       |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-26    |                                |         | TX        |                                   |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-27    | ``m5stack_stamps3_header:26``  |         | HOLD      | Battery-hold signal               |
+----------+--------------------------------+---------+-----------+-----------------------------------+
| M1-28    | **3_3V**                                             | 3.3V main output                  |
|          |                                                      | (via StampS3 DCDC)                |
+----------+--------------------------------+---------+-----------+-----------------------------------+

Start Application Development
*****************************

Before powering up your M5Stack M5Dial, please make sure that the board is in good
condition with no obvious signs of damage.

Building & Flashing
===================

M5Dial is based on ``m5stack_stamps3``.
You may build and flash applications for M5Stack StampS3 as usual (see
:ref:`build_an_application` and :ref:`application_run` for more details), and
include this shield as an overlay.

A good first example to test is :zephyr:code-sample:`lvgl`:
.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: m5stack_stamps3
   :shield: m5stack_m5dial
   :goals: build flash

The baud rate of 921600bps is set by default. If experiencing issues when flashing,
try using different values by using ``--esp-baud-rate <BAUD>`` option during
``west flash`` (e.g. ``west flash --esp-baud-rate 115200``).

.. note::
   Because there currently is missing automatic PWM control or an external backlight
   driver, the display backlight be default is disabled. You may need to enable it
   in your application or by using the PWM shell.

Debugging
---------

M5Stack M5Dial debugging is not supported due to pinout limitations.

Related Documents
*****************

- `M5Stack M5Dial schematic <https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/products/core/M5Dial/Sch_M5Dial.pdf>`_
- `M5Stack M5Dial documentation <https://docs.m5stack.com/en/core/M5Dial>`_
- `ESP32S3 Datasheet <https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf>`_
