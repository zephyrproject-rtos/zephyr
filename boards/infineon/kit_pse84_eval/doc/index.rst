.. zephyr:board:: kit_pse84_eval

Overview
********
The PSOC™ Edge E84 Evaluation Kit (KIT_PSE84_EVAL) enables applications to use the PSOC™ Edge E84 Series
Microcontroller (MCU) together with multiple on-board multimedia, Machine Learning (ML),
and connectivity features including custom MIPI-DSI displays, audio interfaces,
and AIROC™ Wi-Fi and Bluetooth® combo-based connectivity modules.

The PSOC™ Edge E84 MCUs are based on high-performance Arm® Cortex®-M55 including Helium DSP support,
an Ethos™-U55 NPU, and a low-power Arm® Cortex®-M33 paired with Infineon's ultra-low power NNLite
hardware accelerator. They integrate 2.5D graphics accelerators and display interfaces, while
featuring always-on acoustic activity and wake-word detection, efficient HMI operations, and
extended battery life.

The evaluation kit carries a PSOC™ Edge E84 MCU on a SODIMM-based detachable SOM board connected to
the baseboard. The MCU SOM also has 128 MB of QSP| Flash, 1GB of Octal Flash, 128MB of Octal RAM,
PSOC™ 4000T as CAPSENSE™ co-processor, and onboard AIROC™ Wi-Fi and Bluetooth® combo.

Hardware
********
For more information about the PSOC™ Edge E84 MCUs and the PSOC™ Edge E84 Evaluation Kit:

- `PSOC™ Edge E84 Arm® Cortex® Multicore SoC Website`_
- `PSOC™ Edge E84 Evaluation Kit Website`_

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

- PSOC™ Edge E84 base board
- PSOC™ Edge E84 SOM module
- 4.3in capacitive touch display and USB camera module
- USB Type C to Type-C cable
- Two proximity sensor wires
- Four stand-offs for Raspberry Pi compatible display
- Quick start guide

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Please refer to `kit_pse84_eval User Manual Website`_ for more details.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The KIT-PSE84-EVAL includes an onboard programmer/debugger (`KitProg3`_) to provide debugging,
flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and
require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

Please refer to the `ModusToolbox™ software installation guide`_ to install the
Infineon OpenOCD and Edge Protect Security Suite (edgeprotecttools).

Flashing
========
Applications for the ``kit_pse84_eval/pse846gps2dbzc4a/m33`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``kit_pse84_eval/pse846gps2dbzc4a/m55``
board target need to be built using sysbuild to include the required application for the other core.

Enter the following command to compile ``hello_world`` for the CM55 core:

.. code-block:: console

   west build -p -b kit_pse84_eval/pse846gps2dbzc4a/m55 .\samples\hello_world --sysbuild

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
            west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 -p always samples/basic/blinky
            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

            # Do a pristine build once after setting CMake argument
            west build -b kit_pse84_eval/pse846gps2dbzc4a/m33 -p always samples/basic/blinky

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and
perform other standard GDB debugging on the PSOC E84 CM33 core.

References
**********

- `PSOC™ Edge E84 Arm® Cortex® Multicore SoC Website`_

.. _PSOC™ Edge E84 Arm® Cortex® Multicore SoC Website:
    https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm/psoc-edge-e84#Overview

.. _PSOC™ Edge E84 Evaluation Kit Website:
    https://www.infineon.com/evaluation-board/KIT-PSE84-EVAL

.. _kit_pse84_eval User Manual Website:
    https://www.infineon.com/assets/row/public/documents/30/44/infineon-kit-pse84-eval-qsg-usermanual-en.pdf

.. _ModusToolbox™:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxsetup

.. _ModusToolbox™ software installation guide:
    https://www.Infineon.com/ModusToolboxInstallguide

.. _KitProg3:
    https://github.com/Infineon/KitProg3
