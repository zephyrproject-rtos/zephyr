.. zephyr:board:: cy8ckit_062s2_ai

Overview
********

The PSOC 6 AI Evaluation Kit (CY8CKIT-062S2-AI) is a cost effective and small development kit that
enables design and debug of PSOC 6 MCUs.
It includes a CY8C624ABZI-S2D44 MCU which is based on a 150-MHz Arm |reg| Cortex |reg|-M4 and
a 100-MHz Arm |reg| Cortex |reg|-M0+, with 2048 KB of on-chip Flash, 1024 KB of SRAM,
a Quad-SPI external memory interface, built-in hardware and software security features,
rich analog, digital, and communication peripherals.

The board features an AIROC |reg| CYW43439 Wi-Fi & Bluetooth |reg| combo device,
a 512 MB NOR flash, an onboard programmer/debugger (KitProg3), USB host and device features,
two user LEDs, and one push button.

Hardware
********

For more information about the CY8C624ABZI-S2D44 MCU SoC and CY8CKIT-062S2-AI board:

- `CY8C624ABZI-S2D44 MCU SoC Website`_
- `CY8C624ABZI-S2D44 MCU Datasheet`_
- `CY8CKIT-062S2-AI Website`_
- `CY8CKIT-062S2-AI User Guide`_
- `CY8CKIT-062S2-AI Schematics`_

Supported Features
==================

The ``cy8ckit_062s2_ai/cy8c624abzi_s2d44`` board target supports the following hardware features:

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
| PINCTRL   | on-chip    | pin control           |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port-polling;  |
|           |            | serial port-interrupt |
+-----------+------------+-----------------------+
| WATCHDOG  | on-chip    | watchdog              |
+-----------+------------+-----------------------+


The default configuration can be found in the defconfig and dts files:

  - :zephyr_file:`boards/infineon/cy8ckit_062s2_ai/cy8ckit_062s2_ai_defconfig`
  - :zephyr_file:`boards/infineon/cy8ckit_062s2_ai/cy8ckit_062s2_ai.dts`

System Clock
============

The PCY8C624ABZI-S2D44 MCU SoC is configured to use the internal IMO+FLL as a source for
the system clock. CM0+ works at 50MHz, CM4 - at 100MHz. Other sources for the
system clock are provided in the SoC, depending on your system requirements.


Fetch Binary Blobs
******************

The CY8CKIT-062S2-AI board requires fetch binary files (e.g CM0+ prebuilt images).

To fetch Binary Blobs:

.. code-block:: console

   west blobs fetch hal_infineon


Build blinking led sample
*************************

Here is an example for building the :zephyr:code-sample:`blinky` sample application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: cy8ckit_062s2_ai/cy8c624abzi_s2d44
   :goals: build

Programming and Debugging
*************************

The CY8CKIT-062S2-AI board includes an onboard programmer/debugger (`KitProg3`_)
to provide debugging, flash programming, and serial communication over USB.
Flash and debug commands use OpenOCD and require a custom Infineon OpenOCD version,
that supports KitProg3, to be installed.


Infineon OpenOCD Installation
=============================

Both the full `ModusToolbox`_ and the `ModusToolbox Programming Tools`_ packages include Infineon OpenOCD.
Installing either of these packages will also install Infineon OpenOCD.
If neither package is installed, a minimal installation can be done by downloading the `Infineon OpenOCD`_ release
for your system and manually extract the files to a location of your choice.

.. note::

    Linux requires device access rights to be set up for KitProg3.
    This is handled automatically by the ModusToolbox and ModusToolbox Programming Tools installations.
    When doing a minimal installation, this can be done manually by executing the script
    ``openocd/udev_rules/install_rules.sh``.

West Commands
=============

The path to the installed Infineon OpenOCD executable must be available to the ``west`` tool commands.
There are multiple ways of doing this.
The example below uses a permanent CMake argument to set the CMake variable ``OPENOCD``.

   .. tabs::
      .. group-tab:: Windows

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd.exe

            # Do a pristine build once after setting CMake argument
            west build -b cy8ckit_062s2_ai/cy8c624abzi_s2d44 -p always samples/basic/blinky

            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

            # Do a pristine build once after setting CMake argument
            west build -b cy8ckit_062s2_ai/cy8c624abzi_s2d44 -p always samples/basic/blinky

            west flash
            west debug

Alternatively, pyOCD can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner pyocd

References
**********

.. target-notes::

.. _CY8C624ABZI-S2D44 MCU SoC Website:
    https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/psoc-6-32-bit-arm-cortex-m4-mcu/psoc-62/psoc-62x8-62xa/cy8c624abzi-s2d44/

.. _CY8C624ABZI-S2D44 MCU Datasheet:
    https://www.infineon.com/dgdl/Infineon-PSOC_6_MCU_CY8C62X8_CY8C62XA-DataSheet-v16_00-EN.pdf?fileId=8ac78c8c7d0d8da4017d0ee7d03a70b1

.. _CY8CKIT-062S2-AI Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062s2-ai/?redirId=273839

.. _CY8CKIT-062S2-AI User Guide:
    https://www.infineon.com/dgdl/Infineon-CY8CKIT_062S2_AI_KIT_GUIDE-UserManual-v01_00-EN.pdf?fileId=8ac78c8c90530b3a01906d4608842668

.. _CY8CKIT-062S2-AI Schematics:
    https://www.infineon.com/dgdl/Infineon-CY8CKIT-062S2-AI_PSoC_6_AI_Evaluation_Board_Schematic-PCBDesignData-v01_00-EN.pdf?fileId=8ac78c8c8eeb092c018f0af9e109106f

.. _ModusToolbox:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox

.. _ModusToolbox Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
