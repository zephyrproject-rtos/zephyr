.. zephyr:board:: tfpga_s3

Overview
********

The T-FPGA is an M.2-format development module from LILYGO that combines an
Espressif ESP32-S3 microcontroller with a Gowin GW1NSR-LV4 FPGA on a single
carrier. The ESP32-S3 acts as the host processor: it runs Zephyr, manages
power sequencing for the FPGA through an onboard AXP2101 PMIC, and
communicates with the FPGA fabric over a dedicated parallel/SPI link. The
module ships together with a breakout carrier board that exposes four Pmod
connectors, a STEMMA QT/Qwiic connector, a 20-pin expansion header, and an
18650 battery holder.

This board is intended for applications that pair a Wi-Fi/BLE-capable MCU
with reconfigurable logic, such as sensor front-ends, protocol bridges, or
FPGA-accelerated workloads.

Hardware
********

- Espressif ESP32-S3: dual-core Xtensa LX7 @ up to 240 MHz
- 512 KB SRAM, 8 MB PSRAM, 16 MB flash
- Gowin GW1NSR-LV4CQN48PC6/I5 FPGA: 4,608 4-input LUTs, integrated 64 Mb
  PSRAM and 64 Mb HyperRAM, 32 Mb NOR flash
- GWU2X USB-to-JTAG bridge for FPGA configuration and debug
- AXP2101 power management IC, used to sequence and switch the voltage of
  the FPGA's independent I/O bank supplies
- 2.4 GHz Wi-Fi and Bluetooth 5.0 LE
- 2x USB Type-C connectors:
    - ESP32-S3 serial, JTAG and OTG
    - GWU2X USB-JTAG bridge for the FPGA
- QSPI for communication between ESP32-S3 and FPGA
- M.2 Key-B edge connector
- Carrier board
    - 4x Pmod headers
    - 1x STEMMA QT/Qwiic connector
    - 20-pin I/O header
    - 18650 battery holder

Supported Features
==================

.. zephyr:board-supported-hw::

FPGA Power and IO Bank Reference Voltage
========================================

Before the FPGA can be programmed or communicate over USB, the ESP32-S3
must first power up and configure the AXP2101 PMIC so that the FPGA's I/O
banks are supplied with the correct voltages.

Currently, this is handled by the device-tree where 3v3 VDD and 1v2 VCORE
voltages are enabled at boot, but this still required the ESP32 to be powered
on and running a firmware.

For FPGA Bank IO voltages, these need to be enabled on in your zephyr application,
or by using a device tree overlay to enable them at boot.

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

.. _`LILYGO T-FPGA GitHub repository`: https://github.com/Xinyuan-LilyGO/T-FPGA
.. _`LILYGO T-FPGA product page`: https://www.lilygo.cc/products/t-fpga
