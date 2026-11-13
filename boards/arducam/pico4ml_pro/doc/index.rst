.. zephyr:board:: pico4ml_pro

Overview
********

The `Pico4ML-Pro`_ TinyML Dev Kit from Arducam (manufactured by `UCTRONICS`_) is an `RP2040`_ board
with a 2.4-inch touch display, an HM01B0 camera, an IMU, a PDM microphone, a microSD card slot,
an ESP32 for Wi-Fi and four SPI ports for Arducam Mega cameras. It is the successor of the
Arducam Pico4ML.

The RP2040 has two Cortex-M0+ cores. Zephyr currently runs on one core only.

The USB bootloader in the RP2040 boot ROM allows flashing without an adapter, in a drag-and-drop
manner.

Hardware
********

- Raspberry Pi RP2040, dual Arm Cortex-M0+ up to 133 MHz
- 264 KB on-chip SRAM
- 2 MB QSPI flash with XIP
- USB 1.1 device, used for the Zephyr console and for the UF2 bootloader
- 2.4-inch 320x240 TFT display on SPI0, driven as an ST7789V
- XPT2046 resistive touch controller on SPI0
- microSD card slot on SPI0
- Espressif ESP32-D0WDQ6 running Espressif ESP-AT firmware, on UART0
- Himax HM01B0 camera (I2C address 0x24, 1-bit data interface)
- TDK InvenSense ICM-42622 6-axis IMU on I2C0 (address 0x68)
- PDM microphone
- Four 6-pin SPI connectors for Arducam Mega cameras on SPI1

Supported Features
==================

.. zephyr:board-supported-hw::

Wi-Fi
-----

The ESP32 is connected to UART0 and enabled as an ``espressif,esp-at`` device, so
:kconfig:option:`CONFIG_WIFI_ESP_AT` is enabled when :kconfig:option:`CONFIG_WIFI` is set. The
factory ESP32 firmware reports ``AT version:2.4.0.0``. No ESP32 reset or enable line is known to be
connected to the RP2040; the driver resets the ESP32 with ``AT+RST``. The driver disables
auto-connect at startup, so a connection is made again after each reset, for example with
``wifi connect`` in :zephyr:code-sample:`wifi-shell`.

Display and touch
-----------------

The display is the ``zephyr,display`` chosen node and the touch controller the ``zephyr,touch``
chosen node, used by :zephyr:code-sample:`lvgl` as its pointer input. The display is configured in
landscape orientation with RGB565 pixels in big-endian byte order
(``PANEL_PIXEL_FORMAT_RGB_565X``). The touch calibration is taken from the vendor firmware.

microSD card
------------

The card slot is a ``zephyr,sdhc-spi-slot`` with disk name ``SD``, usable with
:zephyr:code-sample:`fs` and :zephyr:code-sample:`shell-fs`. The card detect pin, if any, is not
known.

Not supported yet
-----------------

The IMU, the HM01B0 camera capture, the PDM microphone and the Arducam Mega cameras have no
devicetree nodes on this board yet. The camera control bus (I2C0), the camera clock pin and the
Mega SPI bus (SPI1 with its four chip selects) are configured.

Connections and IOs
===================

No schematic is published. The pin assignments below are taken from the vendor firmware sources
(`Arducam_Mega`_ repository, ``examples/Pico/pico4ml``).

.. list-table::
   :header-rows: 1

   * - Function
     - GPIO
     - Configured in Zephyr
   * - UART0 TX / RX (ESP32 AT port, 115200 baud)
     - GP0 / GP1
     - Yes
   * - HM01B0 VSYNC
     - GP2
     - No
   * - HM01B0 MCLK (PWM slice 1, channel B)
     - GP3
     - No
   * - I2C0 SDA / SCL (HM01B0, ICM-42622)
     - GP4 / GP5
     - Yes
   * - HM01B0 data D0
     - GP6
     - No
   * - microSD CS (SPI0 CS 2)
     - GP7
     - Yes
   * - XPT2046 interrupt
     - GP8
     - Yes
   * - XPT2046 CS (SPI0 CS 1)
     - GP9
     - Yes
   * - SPI1 SCK / MOSI / MISO (Arducam Mega)
     - GP10 / GP11 / GP12
     - Yes
   * - Arducam Mega port CS (SPI1 CS 0 to 3)
     - GP13, GP28, GP17, GP25
     - Yes
   * - HM01B0 PCLK / HSYNC
     - GP14 / GP15
     - No
   * - SPI0 MISO / SCK / MOSI
     - GP16 / GP18 / GP19
     - Yes
   * - Display CS (SPI0 CS 0)
     - GP20
     - Yes
   * - Display DC / reset
     - GP21 / GP22
     - Yes
   * - PDM microphone data / clock
     - GP26 / GP27
     - No

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Zephyr console is the USB CDC ACM port, since UART0 is connected to the ESP32. Output printed
before the host opens the port can be lost; samples that print only at boot are easier to follow
with the shell.

Flashing
========

Hold the BOOTSEL button while connecting the USB cable. The RP2040 boot ROM then appears on the host
as a mass storage device named ``RPI-RP2``. Build and flash an application, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl
   :board: pico4ml_pro
   :goals: build flash

The ``uf2`` runner copies :file:`build/zephyr/zephyr.uf2` to the ``RPI-RP2`` device. Copying the
file by hand works as well.

Debugging
=========

No SWD connector for the RP2040 is documented for this board. Use logging over the USB console.

References
**********

.. target-notes::

.. _Pico4ML-Pro:
   https://docs.arducam.com/Arduino-SPI-camera/Legacy-SPI-camera/Pico/Arducam-Pico4ML-TinyML/Pico4ML-Pro-TinyML-Dev-Kit/

.. _UCTRONICS:
   https://www.uctronics.com/edge-ai/pico4ml/pico4ml-pro-tinyml-dev-kit-plug-and-play-rp2040-board-with-2-4-inch-lcd-screen-built-in-camera-and-microphone-multiple-mega-spi-camera-adapter.html

.. _RP2040:
   https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf

.. _Arducam_Mega:
   https://github.com/ArduCAM/Arducam_Mega
