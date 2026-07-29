.. zephyr:board:: esp32p4x_function_ev_board

Overview
********

ESP32-P4X-Function-EV-Board is an evaluation board from Espressif built around the
ESP32-P4 SoC, a high-performance dual-core RISC-V MCU. The board pairs the bare
SoC with 16 MB of on-board flash and 8 MB of external PSRAM, and exposes USB
Serial/JTAG, MIPI CSI/DSI, SD card and expansion headers for application
development. For more information, check `ESP32-P4X-Function-EV-Board`_.

This board is the ESP32-P4-Function-EV-Board respun with ESP32-P4 silicon
revision v3.1 (the v3.x family). The v3.x parts add the VDD_HP_1 power rail and
require a different PCB and a build targeting revision v3.x (different ROM and
register layout) than the older v1.3 board; the Zephyr board configuration
targets revision v3.1 with the HP cores clocked at 400 MHz. For the older v1.3
board, use ``esp32p4_function_ev_board`` instead.

Hardware
********

ESP32-P4X-Function-EV-Board ships with:

- ESP32-P4 SoC (silicon revision v3.1) paired with 16 MB on-board flash and
  8 MB external PSRAM
- USB-C connector wired to the on-chip USB Serial/JTAG controller
- MIPI CSI camera connector
- MIPI DSI display connector
- microSD card slot
- 10/100 Mbps Ethernet RJ45 port driven by the on-chip EMAC and an
  IP101 PHY over RMII
- Boot and reset buttons
- Expansion headers exposing UART, I2C, SPI and GPIO

.. include:: ../../../espressif/common/soc-esp32p4-features.rst
   :start-after: espressif-soc-esp32p4-features

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`ESP32-P4X-Function-EV-Board`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4x-function-ev-board/user_guide.html
