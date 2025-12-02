.. zephyr:board:: kit_pse84_ai

Overview
********
The PSOC™ Edge E84 AI kit enables evaluation and development of applications using the PSOC™ Edge
E84 Series Microcontroller (MCU) and a multitude of on-board multimedia, Machine Learning (ML), and
connectivity features like Raspberry Pi compatible MIPI-DSI displays, analog and digital microphones
for audio interfaces, and AIROC™ CYW55513IUBGT base Wi-Fi & Bluetooth combo Murata Type2FY
connectivity module.  The kit also has 512-Mbit Quad-SPI NOR Flash and 128-Mbit Octal-SPI HYPERRAM™.
The board features an on-board programmer/debugger (KitProg3), JTAG/SWD debug headers, expansion I/O
header, USB-C connectors, 6-axis IMU sensor, 3-axis magnetometer, barometric pressure sensor,
humidity sensor, RADAR sensor, user LEDs, and a user buttor.  The MCU power domain and perihporal
power domain supports operating voltages of 1.8V and 3.3V.

PSOC™ E84 MCU is an ultra-low-power PSOC™ device specifically designed for ML, wearables and IoT
products like smart thermostats, smart locks, smart home appliances and industrial HMI.

PSOC™ E84 MCU is a true programmable embedded system-on-chip with dual CPUs, integrating a 400 MHz
Arm® Cortex®-M55 as the primary application processor, a 200 MHz Arm® Cortex®-M33 that supports
low-power operations, and a 400 MHz Arm® Ethos-U55 as a neural net companion processor, graphics and
audio block, DSP capability, security enclave with crypto accelerators and protection units,
high-performance memory expansion capability (QSPI, and Octal HYPERRAM™), low-power analog subsystem
with high performance analog-to-digital conversion and low-power comparators, on-board IoT
connectivity module , communication channels, programmable analog and digital blocks that allow
higher flexibility, in-field tuning of the design, and faster time-to-market.

Hardware
********
For more information about the PSOC™ Edge E84 MCUs and the PSOC™ Edge E84 AI Kit:

- `PSOC™ Edge Arm® Cortex® Multicore SoC Website`_
- `PSOC™ Edge E84 AI Kit Website`_

Kit Features:
=============

- Cortex®-M55 CPU with Helium™ DSP
- Advanced ML with Arm Ethos™-U55 NPU
- Low-Power Cortex®-M33
- NNLite ultra-low power NPU
- Analog and Digital Microphones
- State-of-the-Art Secured Enclave
- Integrated Programmer/Debugger

Kit Contents:
=============

- PSOC™ Edge E84 AI board
- OV7675 DVP camera module

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Please refer to `kit_pse84_ai User Manual Website`_ for more details.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The KIT-PSE84-AI includes an onboard programmer/debugger (`KitProg3`_) to provide debugging,
flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and
require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

Please refer to the `ModusToolbox™ software installation guide`_ to install the
Infineon OpenOCD and Edge Protect Security Suite (edgeprotecttools).

Flashing
========
Applications for the ``kit_pse84_ai/pse846gps2dbzc4a/m33`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``kit_pse84_ai/pse846gps2dbzc4a/m55``
board target need to be built using sysbuild to include the required application for the other core.

Enter the following command to compile ``hello_world`` for the CM55 core:

.. code-block:: console

   west build -p -b kit_pse84_ai/pse846gps2dbzc4a/m55 samples/hello_world --sysbuild

Debugging
=========
The path to the installed Infineon OpenOCD executable must be available to the ``west`` tool
commands. There are multiple ways of doing this. The example below uses a permanent CMake argument
to set the CMake variable ``OPENOCD``.

   .. tabs::
      .. group-tab:: Windows

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd.exe

            # Do a pristine build once after setting CMake argument
            west build -b kit_pse84_ai/pse846gps2dbzc4a/m33 -p always samples/basic/blinky
            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

            # Do a pristine build once after setting CMake argument
            west build -b kit_pse84_ai/pse846gps2dbzc4a/m33 -p always samples/basic/blinky

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and
perform other standard GDB debugging on the PSOC E84 CM33 core.

References
**********

- `PSOC™ Edge Arm® Cortex® Multicore SoC Website`_

.. _PSOC™ Edge Arm® Cortex® Multicore SoC Website:
    https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm/psoc-edge-e84#Overview

.. _PSOC™ Edge E84 AI Kit Website:
    https://www.infineon.com/evaluation-board/KIT-PSE84-AI

.. _kit_pse84_ai User Manual Website:
    https://www.infineon.com/assets/row/public/documents/30/44/infineon-kit-pse84-ai-user-guide-usermanual-en.pdf

.. _ModusToolbox™:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxsetup

.. _ModusToolbox™ software installation guide:
    https://www.Infineon.com/ModusToolboxInstallguide

.. _KitProg3:
    https://github.com/Infineon/KitProg3
