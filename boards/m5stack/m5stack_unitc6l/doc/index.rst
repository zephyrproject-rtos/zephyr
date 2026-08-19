.. zephyr:board:: m5stack_unitc6l

Overview
********

M5Stack Unit C6L is a compact development board built around the Stamp C6LoRa module
with an ESP32-C6 SoC and 16 MB SPI flash. It integrates Wi-Fi 6, Bluetooth LE,
IEEE 802.15.4 (Thread/Zigbee), and an SX1262 LoRa transceiver. For more
information, see the `M5Stack Unit C6L documentation`_.

Hardware
********

M5Stack Unit C6L features:

- ESP32-C6 Stamp C6LoRa module (16 MB flash)
- USB Type-C console and programming via the built-in USB Serial/JTAG interface
- 2.4 GHz Wi-Fi 6 and Bluetooth LE
- IEEE 802.15.4 radio for Thread/Zigbee
- SX1262 LoRa transceiver (868/915 MHz)
- 0.66" SSD1306 OLED display (64 x 48, SPI)
- WS2812C RGB LED (GPIO2, I2S)
- Passive buzzer (GPIO11, PWM)
- User button (PI4IOE5V6408 P0)
- HY2.0-4P Grove Port A (5V, GND, GPIO4, GPIO5 - shared with the LP UART)

The OLED shares SPI2 with the SX1262 LoRa modem (MOSI/SCK) and uses a
separate chip select (GPIO6), data/command (GPIO18), and reset (GPIO15).

Grove Port A
============

The port exposes 5V, GND and two general-purpose SoC pins, so it is a multi-purpose
connector: besides the functions wired up below, GPIO4 and GPIO5 can act as plain
GPIO, ADC1 channels 4 and 5, LEDC/PWM outputs, LP GPIO, or the LP UART. Only one
function can be active at a time:

.. list-table::
   :header-rows: 1

   * - Function
     - Node
     - GPIO4 (white)
     - GPIO5 (yellow)
     - Default
   * - I2C (bit-banged)
     - ``&grove_i2c`` / ``zephyr_i2c``
     - SCL
     - SDA
     - enabled
   * - UART
     - ``&grove_uart`` (``uart1``)
     - RXD
     - TXD
     - disabled
   * - GPIO
     - ``&grove_header`` pins 0 and 1
     - pin 0
     - pin 1
     - \-

The only hardware I2C controller is used by the on-board IO expander, so the
port uses the ``gpio-i2c`` bit-banged driver. To use the port as a serial line
instead, swap the two nodes in an application overlay:

.. code-block:: devicetree

   &grove_i2c {
           status = "disabled";
   };

   &grove_uart {
           status = "okay";
           current-speed = <115200>;
   };

LP UART pin sharing
-------------------

The ESP32-C6 LP UART is hard-wired to GPIO4 (RXD) and GPIO5 (TXD) - the same pins
routed to Grove Port A - and cannot be remapped through pinctrl. The ``lpcore``
variant enables ``&lp_uart`` and selects it as console and shell, but the LP core
image only consumes the peripheral: clock setup and the RTC IO muxing that routes the
signals to the pads are done by the LP UART driver on the HP core.

An LP core build that needs the LP UART therefore also depends on the HP core image.
Release the Grove pins and enable ``&lp_uart`` in an HP core overlay, so its init runs
and claims the pads:

.. code-block:: devicetree

   /* HP core: release the Grove pins and route the LP UART pads */
   &grove_i2c {
           status = "disabled";
   };

   &grove_uart {
           status = "disabled";
   };

   &lp_uart {
           status = "okay";
   };

While the LP UART is in use, Grove Port A is no longer available to the HP core for
I2C, UART or GPIO.

.. include:: ../../../espressif/common/soc-esp32c6-features.rst
   :start-after: espressif-soc-esp32c6-features

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

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`M5Stack Unit C6L documentation`: https://docs.m5stack.com/en/unit/Unit_C6L
