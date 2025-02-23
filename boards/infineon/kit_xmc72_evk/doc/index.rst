.. zephyr:board:: kit_xmc72_evk

Overview
********

The XMC7200 evaluation kit enables you to evaluate and develop your applications using the XMC7200D microcontroller(hereafter called “XMC7200D”). XMC7200D is designed for industrial applications. XMC7200D is a true programmable embedded system-on chip, integrating up to two 350-MHz Arm® Cortex®-M7 as the primary application processors, 100-MHz Arm® Cortex®-M0+ that supports low-power operations, up to 8-MB flash and 1-MB SRAM, Gigabit Ethernet, Controller Area Network Flexible Data-Rate (CAN FD), Secure Digital Host Controller (SDHC) supporting SD/SDIO/eMMC interfaces, programmable analog and digital peripherals that allow faster time-to-market. The evaluation board has a M.2 interface connector for interfacing radio modules-based on AIROC™ Wi-Fi & Bluetooth combos, SMIF dual header compatible with Digilent Pmod for interfacing HYPERBUS™ memories, and headers compatible with Arduino for interfacing Arduino shields. In addition, the board features an onboard programmer/debugger(KitProg3), a 512-Mbit QSPI NOR flash, CAN FD transceiver, Gigabit Ethernet PHY transceiver with RJ45 connector interface, a micro-B connector for USB device interface, three user LEDs, one potentiometer, and two push buttons. The board supports operating voltages from 3.3 V to 5.0 V for XMC7200D.

Hardware
********

For more information about XMC7200D and KIT_XMC72_EVK:

- `XMC7200D SoC Website`_
- `kit_xmc72_evk Board Website`_

Kit Features:
=============

- Evaluation board for XMC7200D-E272K8384 in BGA package with 272 pins dual-core Arm®Cortex® -M7 CPUs running at - 50-MHz and an Arm® Cortex®-M0+ CPU running at 100-MHz
- Full-system approach on the board, featuring Gigabit Ethernet PHY and connector, CAN FD transceiver, user LEDs, - uttons, and potentiometer
- M.2 interface connector for interfacing radio modules based on AIROC™ Wi-Fi & Bluetooth®combos (currently not - supported)
- Headers compatible with Arduino for interfacing Arduino shields
- Fully compatible with ModusToolbox™ v3.0
- KitProg3 on-board SWD programmer/debugger, USB-UART, and USB-I2C bridge functionality through USB connector
- Digilent dual PMOD SMIF header for interfacing HYPERBUS™ memories (currently not supported)
- A 512-Mbit external QSPI NOR flash
- Evaluation board supports operating voltages from 3.3 V to 5.0 V for XMC7200D

Kit Contents:
=============

- XMC7200 evaluation board
- USB Type-A to Mirco-B cable
- 12V/3A DC power adapter with additional blades
- Six jumper wires (five inches each)
- Quick start guide

Build blinking led sample
*************************

Here is an example for building the :zephyr:code-sample:`blinky` sample application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: kit_xmc72_evk
   :goals: build

Infineon OpenOCD Installation
=============================

Both the full `ModusToolbox`_ and the `ModusToolbox Programming Tools`_ packages include Infineon OpenOCD. Installing either of these packages will also install Infineon OpenOCD. If neither package is installed, a minimal installation can be done by downloading the `Infineon OpenOCD`_ release for your system and manually extract the files to a location of your choice.

.. note:: Linux requires device access rights to be set up for KitProg3. This is handled automatically by the ModusToolbox and ModusToolbox Programming Tools installations. When doing a minimal installation, this can be done manually by executing the script ``openocd/udev_rules/install_rules.sh``.

.. _XMC7200D SoC Website:
    https://www.infineon.com/cms/en/product/microcontroller/32-bit-industrial-microcontroller-based-on-arm-cortex-m/32-bit-xmc7000-industrial-microcontroller-arm-cortex-m7/xmc7200d-e272k8384aa/

.. _kit_xmc72_evk Board Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/kit_xmc72_evk

.. _ModusToolbox:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox

.. _ModusToolbox Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
