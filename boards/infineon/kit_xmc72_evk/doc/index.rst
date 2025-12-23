.. zephyr:board:: kit_xmc72_evk

Overview
********

The XMC7200 evaluation kit enables you to evaluate and develop your applications using the XMC7200D
microcontroller(hereafter called “XMC7200D”). The XMC7200D is designed for industrial applications
and it is a true programmable embedded system-on-chip, integrating up to two 350-MHz Arm® Cortex®-M7
as the primary application processor, a 100-MHz Arm® Cortex®-M0+ that supports the following:

- Low-power operations
- Up to 8 MB flash and 1 MB SRAM
- Gigabit Ethernet
- CAN FD
- Secure Digital Host Controller (SDHC) supporting SD/SDIO/eMMC interfaces
- Programmable analog and digital peripherals that allow faster time-to-market

The evaluation board has a M.2 interface connector for interfacing radio modules-based on
AIROC™ Wi-Fi & Bluetooth combos, SMIF dual header compatible with Digilent Pmod for interfacing
HYPERBUS™ memories, and headers compatible with Arduino for interfacing Arduino shields.
In addition, the board features an onboard programmer/debugger(KitProg3), a 512-Mbit QSPI NOR flash,
CAN FD transceiver, Gigabit Ethernet PHY transceiver with RJ45 connector interface, a micro-B
connector for USB device interface, three user LEDs, one potentiometer, and two push buttons.
The board supports operating voltages from 3.3 V to 5.0 V for XMC7200D.

Hardware
********

For more information about XMC7200D and KIT_XMC72_EVK:

- `XMC7200D SoC Website`_
- `kit_xmc72_evk Board Website`_

Kit Features
=============

- Evaluation board for XMC7200D-E272K8384 in BGA package with 272 pins, dual-core Arm®Cortex®-M7 CPUs running at 350-MHz and an Arm® Cortex®-M0+ CPU running at 100-MHz
- Full-system approach on the board, featuring Gigabit Ethernet PHY and connector, CAN FD transceiver, user LEDs, buttons, and potentiometer
- M.2 interface connector for interfacing radio modules based on AIROC™ Wi-Fi & Bluetooth®combos (currently not - supported)
- Headers compatible with Arduino for interfacing Arduino shields
- Fully compatible with ModusToolbox™ v3.0
- KitProg3 on-board SWD programmer/debugger, USB-UART, and USB-I2C bridge functionality through USB connector
- Digilent dual PMOD SMIF header for interfacing HYPERBUS™ memories (currently not supported)
- A 512-Mbit external QSPI NOR flash
- Evaluation board supports operating voltages from 3.3 V to 5.0 V for XMC7200D

Kit Contents
=============

- XMC7200 evaluation board
- USB Type-A to Mirco-B cable
- 12V/3A DC power adapter with additional blades
- Six jumper wires (five inches each)
- Quick start guide

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Here is an example for building the :zephyr:code-sample:`blinky` sample application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: kit_xmc72_evk
   :goals: build

Flashing
========

The KIT_XMC72_EVK includes an onboard programmer/debugger (`KitProg3`_) to provide debugging, flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

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
            west build -b kit_xmc72_evk -p always samples/basic/blinky

            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

            # Do a pristine build once after setting CMake argument
            west build -b kit_xmc72_evk -p always samples/basic/blinky

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and perform other standard GDB debugging.

References
**********

.. target-notes::

.. _XMC7200D SoC Website:
    https://www.infineon.com/evaluation-board/KIT-XMC72-EVK

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
