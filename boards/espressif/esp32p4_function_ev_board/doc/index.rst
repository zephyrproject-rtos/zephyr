.. zephyr:board:: esp32p4_function_ev_board

Overview
********

ESP32-P4-Function-EV-Board is an evaluation board from Espressif built around the
ESP32-P4 SoC, a high-performance dual-core RISC-V MCU running up to 400 MHz. The
board pairs the bare SoC with 16 MB of on-board flash and 8 MB of external PSRAM,
and exposes USB Serial/JTAG, MIPI CSI/DSI, SD card and expansion headers
for application development. For more information, check
`ESP32-P4-Function-EV-Board`_.

Hardware
********

ESP32-P4-Function-EV-Board ships with:

- ESP32-P4 SoC paired with 16 MB on-board flash and 8 MB external PSRAM
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

.. _`ESP32-P4-Function-EV-Board`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html
