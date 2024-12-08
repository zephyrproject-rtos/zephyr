.. zephyr:board:: cyw920829m2evk_02

Overview
********

The AIROC™ CYW20829 Bluetooth® LE MCU Evaluation Kit (CYW920829M2EVK-02) with its included on-board peripherals enables evaluation, prototyping, and development of a wide array of Bluetooth® Low Energy applications, all on Infineon's low power, high performance AIROC™ CYW20829. The AIROC™ CYW20829's robust RF performance and 10 dBm TX output power without an external power amplifier (PA). This provides enough link budget for the entire spectrum of Bluetooth® LE use cases including industrial IoT applications, smart home, asset tracking, beacons and sensors, and medical devices.

The system features Dual Arm® Cortex® - M33s for powering the MCU and Bluetooth subsystem with programmable and reconfigurable analog and digital blocks. In addition, on the kit, there is a suite of on-board peripherals including six-axis inertial measurement unit (IMU), thermistor, analog mic, user programmable buttons (2), LEDs (2), and RGB LED. There is also extensive GPIO support with extended headers and Arduino Uno R3 compatibility for third-party shields.

Hardware
********

For more information about the CYW20829 SoC and CYW920829M2EVK-02 board:

- `CYW20829 SoC Website`_
- `CYW920829M2EVK-02 Board Website`_

Kit Features:
=============

- AIROC™ CYW20829 Bluetooth® LE MCU in 56 pin QFN package
- Arduino compatible headers for hardware expansion
- On-board sensors - 6-axis IMU, Thermistor, Infineon analog microphone, and Infineon digital microphone
- User switches, RGB LED and user LEDs
- USB connector for power, programming and USB-UART bridge

Kit Contents:
=============

- CYW20829 evaluation board (CYW9BTM2BASE3+CYW920829M2IPA2)
- USB Type-A to Micro-B cable
- Six jumper wires (five inches each)
- Quick start guide


Supported Features
==================

The board configuration supports the following hardware features:

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| NVIC      | on-chip    | nested vectored       |
|           |            | interrupt controller  |
+-----------+------------+-----------------------+
| SYSTICK   | on-chip    | system clock          |
+-----------+------------+-----------------------+
| GPIO      | on-chip    | GPIO                  |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port-polling;  |
|           |            | serial port-interrupt |
+-----------+------------+-----------------------+


The default configuration can be found in the Kconfig

:zephyr_file:`boards/infineon/cyw920829m2evk_02/cyw920829m2evk_02_defconfig`

System Clock
============

The AIROC™ CYW20829 Bluetooth®  MCU SoC is configured to use the internal IMO+FLL as a source for
the system clock. Other sources for the system clock are provided in the SOC, depending on your
system requirements.

Fetch Binary Blobs
******************

cyw920829m2evk_02 board requires fetch binary files (e.g Bluetooth controller firmware).

To fetch Binary Blobs:

.. code-block:: console

   west blobs fetch hal_infineon

Build blinking led sample
*************************

Here is an example for building the :zephyr:code-sample:`blinky` sample application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: cyw920829m2evk_02
   :goals: build

Programming and Debugging
*************************

The CYW920829M2EVK-02 includes an onboard programmer/debugger (`KitProg3`_) to provide debugging, flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

The CYW920829M2EVK-02 supports RTT via a SEGGER JLink device, under the target name cyw20829_tm. This can be enabled for an application by building with the rtt-console snippet or setting the following config values: CONFIG_UART_CONSOLE=n, CONFIG_RTT_CONSOLE=y, and CONFIG_USE_SEGGER_RTT=y.
e.g. west build -p always -b cyw920829m2evk_02 samples/basic/blinky -S rtt-console

As an additional note there is currently a discrepancy in RAM address between SEGGER and the CYW920829M2EVK-02 device. So, for RTT control block, do not use "Auto Detection". Instead, set the search range to something reflecting: RAM RangeStart at 0x20000000 and RAM RangeSize of 0x3d000.

Infineon OpenOCD Installation
=============================

Both the full `ModusToolbox`_ and the `ModusToolbox Programming Tools`_ packages include Infineon OpenOCD. Installing either of these packages will also install Infineon OpenOCD. If neither package is installed, a minimal installation can be done by downloading the `Infineon OpenOCD`_ release for your system and manually extract the files to a location of your choice.

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
            west build -b cyw920829m2evk_02 -p always samples/basic/blinky

            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

            # Do a pristine build once after setting CMake argument
            west build -b cyw920829m2evk_02 -p always samples/basic/blinky

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and perform other standard GDB debugging on the CYW20829 CM33 core.

.. _CYW20829 SoC Website:
    https://www.infineon.com/cms/en/product/wireless-connectivity/airoc-bluetooth-le-bluetooth-multiprotocol/airoc-bluetooth-le/cyw20829/

.. _CYW920829M2EVK-02 Board Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/cyw920829m2evk-02/

.. _CYW920829M2EVK-02 BT User Guide:
    https://www.infineon.com/cms/en/product/wireless-connectivity/airoc-bluetooth-le-bluetooth-multiprotocol/airoc-bluetooth-le/cyw20829/#!?fileId=8ac78c8c8929aa4d018a16f726c46b26

.. _ModusToolbox:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox

.. _ModusToolbox Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
