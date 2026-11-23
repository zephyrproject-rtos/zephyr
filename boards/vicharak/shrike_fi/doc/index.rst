.. zephyr:board:: shrike_fi

Overview
********

`Shrike-fi`_ is a low-cost, open-source microcontroller + FPGA development board
combining an `ESP32-S3FN8`_ MCU and a 1120 LUT FPGA. It features a PMOD connector,
Qwiic connector, breadboard-compatible layout, integrated FPGA-MCU IO interface,
dual user LEDs, Wi-Fi, Bluetooth LE, and dual USB Type-C ports for power and
programming.

Hardware
********

- Microcontroller `ESP32-S3FN8`_, dual-core Xtensa LX7 up to 240 MHz
- 512 kByte SRAM
- 8 Mbyte in-package flash
- GPIO
- I2C (Qwiic, GPIO7 SDA / GPIO6 SCL)
- SPI (FPGA configuration and MCU-FPGA link)
- UART (console on UART0 via CH9102 USB-UART)
- USB Type-C connectors (UART and native USB)
- Dual user LEDs
- Reset and boot buttons
- 1120 LUT SLG47910 Low-Power FPGA
- Blue LED on GPIO21
- Optional 8 MB QSPI PSRAM (not populated on the base board)

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

MCU user LED is connected to GPIO21 (active high). FPGA user LED is connected
to FPGA GPIO16.

FPGA configuration uses SPI2:

=======  ========  ========================
ESP32    FPGA      Function
=======  ========  ========================
GPIO9    EN        FPGA enable
GPIO8    PWR       FPGA power
GPIO12   3         SPI SCLK
GPIO10   4         SPI SS
GPIO11   5         SPI MOSI
GPIO13   6         SPI MISO / CONFIG
=======  ========  ========================

Qwiic I2C defaults to ESP32 GPIO6 (SCL) and GPIO7 (SDA). UART1 is available on
header pins GPIO17 (TX) and GPIO18 (RX).

The CH9102 USB-UART bridge is wired to UART0 (GPIO43 TX, GPIO44 RX) and is the
default Zephyr console. Native USB Serial/JTAG uses GPIO19/20 on the second
USB Type-C port.

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

The ``shrike_fi`` board is supported by the following runners:

.. zephyr:board-supported-runners::

Connect the USB Type-C port that is wired to the CH9102 UART bridge. Auto-reset
via DTR/RTS is supported; if flashing does not start, hold the boot button while
resetting the board.

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

.. _Shrike-fi:
   https://vicharak-in.github.io/shrike/introduction.html

.. _Shrike-fi Hardware Overview:
   https://vicharak-in.github.io/shrike/hardware_overview.html

.. _Shrike Pinouts:
   https://vicharak-in.github.io/shrike/shrike_pinouts.html

.. _Shrike Source Repository:
   https://github.com/vicharak-in/shrike

.. _ESP32-S3FN8:
   https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
