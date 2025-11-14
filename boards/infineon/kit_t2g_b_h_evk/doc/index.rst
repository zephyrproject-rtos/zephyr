.. zephyr:board:: kit_t2g_b_h_evk

Overview
********

The KIT_T2G-B-H_EVK kit enables you to evaluate and develop applications using the TRAVEO™ T2G Body
High family CYT4BF device. The TRAVEO™ T2G B-H MCU is specifically designed for automotive
applications and it is a true programmable embedded system-on-chip, integrating two 350-MHz Arm®
Cortex®-M7 as the primary application processor, a 100-MHz Arm® Cortex®-M0+ that supports the
following:

- Low-power operations
- Up to 8 MB flash and 1 MB SRAM
- Secure Digital Host Controller (SDHC) supporting SD/SDIO/eMMC interfaces
- Programmable analog and digital peripherals that allow faster time-to-market

The evaluation board carries a TRAVEO™ T2G-B-H MCU, an M.2 interface connector for interfacing radio
modules based on AIROC™ Wi-Fi & Bluetooth® combos (currently not supported), a Dual-PMOD SMIF
connector for interfacing HYPERBUS™ memories, and Arduino headers for interfacing Arduino Shields.
In addition, the board features an on-board programmer/debugger (KitProg3), a 512-Mb QSPI NOR flash,
a micro-B connector for the USB device interface, three user LEDs, one potentiometer, a Gigabit
ethernet port for ethernet applications and two push buttons. The board supports operating voltages
from 1.8 V to 5.0 V for TRAVEO™ T2G-B-H MCU.

Hardware
********

For more information about KIT_T2G-B-H_EVK:

- `kit_t2g_b_h_evk Board Website`_
- `T2G_B_H SoC Website`_

Kit Features
=============

- Evaluation board for CYT4BF MCU4 in BGA package with 272 pins, dual-core Arm®Cortex® M7 CPUs running at 350-MHz and an Arm® Cortex® M0+ CPU running at 100-MHz
- Full-system approach on the board, featuring Gigabit Ethernet PHY and connector, CAN FD transceiver, user LEDs, buttons, and potentiometer
- M.2 interface connector for interfacing radio modules based on AIROC™ Wi-Fi & Bluetooth®combos (currently not - supported)
- Headers compatible with Arduino for interfacing Arduino shields
- Fully compatible with ModusToolbox™ v3.0
- KitProg3 on-board SWD programmer/debugger, USB-UART, and USB-I2C bridge functionality through USB connector
- Digilent dual PMOD SMIF header for interfacing HYPERBUS™ memories (currently not supported)
- A 512-Mbit external QSPI NOR flash
- Evaluation board supports operating voltages from 3.3 V to 5.0 V for CYT4BF

Kit Contents
=============

- TRAVEO™ T2G-B-H evaluation board
- USB Type-A to Micro-B cable
- 12V/3A DC power adapter with additional blades
- Quick start guide

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Here is an example for building the :zephyr:code-sample:`blinky` sample application for Cortex®-M0+.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: kit_t2g_b_h_evk/cyt4bfbche/m0p
   :goals: build

The same for the first Cortex®-M7 core:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: kit_t2g_b_h_evk/cyt4bfbche/m7_0
   :goals: build

And the second Cortex®-M7 core:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: kit_t2g_b_h_evk/cyt4bfbche/m7_1
   :goals: build

.. note:: Only Cortex®-M0+ core is enabled at startup. To enable the first Cortex®-M7 core, add the next code to Cortex®-M0+ application: ``Cy_SysEnableCM7(CORE_CM7_0, CY_CORTEX_M7_0_APPL_ADDR);``, and the next code to enable the second Cortex®-M7 core: ``Cy_SysEnableCM7(CORE_CM7_1, CY_CORTEX_M7_1_APPL_ADDR);``.

Flashing
========

The KIT_T2G_B_H_EVK includes an onboard programmer/debugger (`KitProg3`_) to provide debugging, flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

Infineon OpenOCD Installation
=============================

Both the full `ModusToolbox`_ and the `ModusToolbox Programming Tools`_ packages include Infineon OpenOCD.
Installing either of these packages will also install Infineon OpenOCD.

If neither package is installed, a minimal installation can be done by downloading the `Infineon OpenOCD`_ release for your system and manually extract the files to a location of your choice.

.. note:: Linux requires device access rights to be set up for KitProg3. This is handled automatically by the ModusToolbox and ModusToolbox Programming Tools installations. When doing a minimal installation, this can be done manually by executing the script ``openocd/udev_rules/install_rules.sh``.

West Commands
=============

The path to the installed Infineon OpenOCD executable must be available to the ``west`` tool commands. There are multiple ways of doing this. The example below uses a permanent CMake argument to set the CMake variable ``OPENOCD``.

   .. tabs::
      .. group-tab:: Windows

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd.exe

            # Do a pristine build once after setting CMake argument
            west build -b kit_t2g_b_h_evk/cyt4bfbche/m0p -p always samples/basic/blinky

            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

            # Do a pristine build once after setting CMake argument
            west build -b kit_t2g_b_h_evk/cyt4bfbche/m0p -p always samples/basic/blinky

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and perform other standard GDB debugging.

References
**********

.. target-notes::

.. _T2G_B_H SoC Website:
    https://www.infineon.com/products/microcontroller/32-bit-traveo-t2g-arm-cortex/for-body/t2g-cyt4bf

.. _kit_t2g_b_h_evk Board Website:
    https://www.infineon.com/evaluation-board/KIT-T2G-B-H-EVK

.. _ModusToolbox:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox

.. _ModusToolbox Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
