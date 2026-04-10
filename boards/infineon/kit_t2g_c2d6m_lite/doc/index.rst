.. zephyr:board:: kit_t2g_c2d6m_lite

Overview
********

The KIT_T2G-C-2D-6M_LITE kit enables you to evaluate and develop applications using the TRAVEO™ T2G
Cluster 2D family CYT4DN device. The TRAVEO™ T2G CYT4DN MCU is specifically designed for automotive
instrument cluster and Head-Up Display (HUD) applications and it is a true programmable embedded
system-on-chip, integrating two 320-MHz Arm® Cortex®-M7 CPUs as the primary application processors,
a 100-MHz Arm® Cortex®-M0+ that supports the following:

- Low-power operations (Active, Sleep, Low-power Sleep, Deep Sleep, and Hibernate modes)
- 6336 KB code-flash with additional 128 KB work-flash and 640 KB SRAM
- Programmable analog and digital peripherals that allow faster time-to-market

Hardware
********

For more information about KIT_T2G-C-2D-6M_LITE:

- `kit_t2g_c2d6m_lite Board Website`_
- `T2G_C_2D SoC Website`_

Kit Features
============

- Evaluation board for CYT4DN MCU in 327-BGA package, dual-core Arm® Cortex® M7 CPUs running at 320-MHz and an Arm® Cortex® M0+ CPU running at 100-MHz
- Full-system approach on the board, featuring Gigabit Ethernet PHY and connector, CAN FD transceiver, user LEDs, buttons, and potentiometer
- Headers compatible with Arduino for interfacing Arduino shields
- Fully compatible with ModusToolbox™ v3.0
- KitProg3 on-board SWD programmer/debugger, USB-UART, and USB-I2C bridge functionality through USB connector
- Evaluation board supports operating voltages from 3.3 V to 5.0 V for CYT4DN

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Build the :zephyr:code-sample:`hello_world` sample application for the Cortex®-M0+ core:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_t2g_c2d6m_lite/cyt4dnjbzs/m0p
   :goals: build

To release the Cortex®-M7 cores from M0+ during boot, pass the corresponding Kconfig option(s) at build time. Only enable a core when valid firmware is present at its flash address; otherwise the core will fetch invalid instructions and fault.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_t2g_c2d6m_lite/cyt4dnjbzs/m0p
   :gen-args: -DCONFIG_SOC_CYT4DN_START_M7_0=y -DCONFIG_SOC_CYT4DN_START_M7_1=y
   :goals: build

Build for the Cortex®-M7 Core 0:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_t2g_c2d6m_lite/cyt4dnjbzs/m7_0
   :goals: build

Build for the Cortex®-M7 Core 1:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_t2g_c2d6m_lite/cyt4dnjbzs/m7_1
   :goals: build

.. note:: Per the CYT4DN start-up sequence, only the Cortex®-M0+ core is released from reset by ROM/flash boot. The Cortex®-M0+ application is responsible for enabling the Cortex®-M7 cores: it sets ``CLK_HF1`` for CM7_0 and CM7_1, programs the vector table bases (``CM7_0_VECTOR_TABLE_BASE`` at ``0x40200200`` and ``CM7_1_VECTOR_TABLE_BASE`` at ``0x40200600``), enables power for both M7 cores, clears ``CPU_WAIT``, and releases the cores from reset.

Flashing
========

The KIT_T2G_C-2D-6M_LITE includes an onboard programmer/debugger (`KitProg3`_) to provide debugging, flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

Both the full `ModusToolbox`_ and the `ModusToolbox Programming Tools`_ packages include Infineon OpenOCD. Installing either of these packages will also install Infineon OpenOCD. If neither package is installed, a minimal installation can be done by downloading the `Infineon OpenOCD`_ release for your system and manually extract the files to a location of your choice.

.. note:: Linux requires device access rights to be set up for KitProg3. This is handled automatically by the ModusToolbox and ModusToolbox Programming Tools installations. When doing a minimal installation, this can be done manually by executing the script ``openocd/udev_rules/install_rules.sh``.

The path to the installed Infineon OpenOCD executable must be available to the ``west`` tool commands. The examples below use a permanent CMake argument to set the CMake variable ``OPENOCD``.

Set the OpenOCD path once:

.. code-block:: shell

   west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

Build and flash for the Cortex®-M0+ core:

.. code-block:: shell

   west build -b kit_t2g_c2d6m_lite/cyt4dnjbzs/m0p -p always samples/hello_world
   west flash
   west debug

Build and flash for the Cortex®-M0+ core with M7 cores released during boot:

.. code-block:: shell

   west build -b kit_t2g_c2d6m_lite/cyt4dnjbzs/m0p -p always samples/hello_world \
       -- -DCONFIG_SOC_CYT4DN_START_M7_0=y -DCONFIG_SOC_CYT4DN_START_M7_1=y
   west flash
   west debug

Build and flash for the Cortex®-M7 Core 0:

.. code-block:: shell

   west build -b kit_t2g_c2d6m_lite/cyt4dnjbzs/m7_0 -p always samples/hello_world
   west flash
   west debug

Build and flash for the Cortex®-M7 Core 1:

.. code-block:: shell

   west build -b kit_t2g_c2d6m_lite/cyt4dnjbzs/m7_1 -p always samples/hello_world
   west flash
   west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and perform other standard GDB debugging.

References
**********

.. target-notes::

.. _T2G_C_2D SoC Website:
   https://www.infineon.com/products/microcontroller/32-bit-traveo-t2g-arm-cortex/for-cluster/t2g-cyt4dn

.. _kit_t2g_c2d6m_lite Board Website:
   https://www.infineon.com/evaluation-board/KIT-T2G-C-2D-6M-LITE

.. _ModusToolbox:
   https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox

.. _ModusToolbox Programming Tools:
   https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
   https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
   https://github.com/Infineon/KitProg3
