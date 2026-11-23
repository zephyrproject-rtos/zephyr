.. zephyr:board:: m5stack_stackchan

Overview
********

M5Stack StackChan is a desktop robot from M5Stack based on the open-source Stack-chan
project by Shishikawa. It consists of the *StackChan Core* main unit (an ESP32-S3
controller that shares its design with the M5Stack CoreS3) and a robot body that
holds the battery, two serial servos, LEDs, a touch panel, IR and NFC. See the
`M5Stack StackChan Documentation`_ and the `StackChan Core Documentation`_.

(Image courtesy of M5Stack, taken from the `M5Stack StackChan product page`_.)

Hardware
********

StackChan Core (main unit):

- ESP32-S3 chip (dual-core Xtensa LX7 processor @240MHz, WIFI, OTG and CDC functions)
- PSRAM 8MB (Quad)
- Flash 16MB
- LCD IPS 2", 320x240 pixel ILI9342C
- Capacitive multi touch FT6336U
- Speaker 1W AW88298
- Dual microphones with ES7210 audio ADC
- RTC BM8563
- PMIC AXP2101
- GPIO expander AW9523B
- Camera 0.3 MP GC0308
- Geomagnetic sensor BMM150
- Proximity sensor LTR-553ALS-WA
- 6-Axis IMU BMI270
- microSD card slot
- USB-C

Robot body:

- Battery 550mAh 3.7 V with INA226 battery monitor
- Two SCS0009 serial servos (yaw / pitch) on UART1 (GPIO6 TX / GPIO7 RX, 1 Mbps half-duplex)
- IO expander PY32L020 (servo power enable, 12 RGB LEDs)
- Touch panel Si12T (3 zones)
- IR transmitter (GPIO5) and IRM56384 IR receiver (GPIO10)
- NFC ST25R3916
- Three Grove (HY2.0-4P) ports: PORT.A (I2C), PORT.B (GPIO / ADC), PORT.C (UART)

The M-Bus of the main unit is passed through to the robot body, so M-Bus
modules can be stacked between the two as long as they do not use the pins
taken by the body (see below).

Grove ports
===========

.. list-table::
   :header-rows: 1

   * - Port
     - Pins
     - Zephyr node
   * - PORT.A
     - GPIO1 (SCL), GPIO2 (SDA)
     - ``i2c1`` (``zephyr_i2c`` / ``grove_header``)
   * - PORT.B
     - GPIO8, GPIO9
     - ``gpio0`` (also usable as ADC inputs)
   * - PORT.C
     - GPIO17 (TX), GPIO18 (RX)
     - ``uart2`` (``grove_uart``), disabled by default

UART1 (GPIO6 / GPIO7) is wired to the servos inside the body and is not
available on any port. The 5 V supply of the Grove ports is controlled by the
``bus_5v`` regulator.

M-Bus pins used by the body
===========================

.. list-table::
   :header-rows: 1

   * - Pins
     - Used for
   * - GPIO6, GPIO7
     - Servo bus (``uart1``)
   * - GPIO11, GPIO12
     - Internal I2C (``i2c0``): INA226 (0x41), ST25R3916 (0x50), Si12T (0x68),
       PY32L020 (0x6F)
   * - GPIO5, GPIO10
     - IR transmitter / receiver
   * - GPIO17, GPIO18
     - PORT.C (``uart2``, ``m5stack_mbus_uart1``)
   * - GPIO8, GPIO9
     - PORT.B
   * - GPIO1, GPIO2
     - PORT.A (``i2c1``)
   * - 5V, BAT, GND
     - Power

A module stacked between the main unit and the body must not drive these
pins; for example, a module using GPIO6 / GPIO7 conflicts with the servos.

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

The board has no GPIO-connected LED, so :zephyr:code-sample:`blinky` does not
apply. The ``led0`` alias points to the AXP2101 status LED, which is driven
through the LED API; it can be exercised from the shell with
``CONFIG_LED_SHELL=y`` (``led on led 0``).

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`M5Stack StackChan Documentation`: https://docs.m5stack.com/en/StackChan
.. _`M5Stack StackChan product page`: https://shop.m5stack.com/products/stackchan-kawaii-co-created-open-source-ai-desktop-robot
.. _`StackChan Core Documentation`: https://docs.m5stack.com/en/core/StackChan_Core
