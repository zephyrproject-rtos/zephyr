.. _mr_canhubk3:

NXP MR-CANHUBK3
###############

Overview
********

`NXP MR-CANHUBK3`_ is an evaluation board for mobile robotics applications such
as autonomous mobile robots (AMR) and automated guided vehicles (AGV). It
features an `NXP S32K344`_ general-purpose automotive microcontroller based on
an Arm Cortex-M7 core (Lock-Step).

.. image:: img/mr_canhubk3_top.jpg
     :align: center
     :alt: NXP MR-CANHUBK3 (TOP)

Hardware
********

- NXP S32K344
    - Arm Cortex-M7 (Lock-Step), 160 MHz (Max.)
    - 4 MB of program flash, with ECC
    - 320 KB RAM, with ECC
    - Ethernet 100 Mbps, CAN FD, FlexIO, QSPI
    - 12-bit 1 Msps ADC, 16-bit eMIOS timer

- `NXP FS26 Safety System Basis Chip`_

- Interfaces:
    - Console UART
    - 6x CAN FD
    - 100Base-T1 Ethernet
    - DroneCode standard JST-GH connectors and I/O headers for I2C, SPI, GPIO,
      PWM, etc.

More information about the hardware and design resources can be found at
`NXP MR-CANHUBK3`_ website.

Supported Features
==================

The ``mr_canhubk3`` board configuration supports the following hardware features:

============  ==========  ================================
Interface     Controller  Driver/Component
============  ==========  ================================
SIUL2         on-chip     | pinctrl
                          | gpio
                          | external interrupt controller
LPUART        on-chip     serial
============  ==========  ================================

The default configuration can be found in the Kconfig file
:zephyr_file:`boards/arm/mr_canhubk3/mr_canhubk3_defconfig`.

Connections and IOs
===================

Each GPIO port is divided into two banks: low bank, from pin 0 to 15, and high
bank, from pin 16 to 31. For example, ``PTA2`` is the pin 2 of ``gpioa_l`` (low
bank), and ``PTA20`` is the pin 4 of ``gpioa_h`` (high bank).

LEDs
----

The MR-CANHUBK3 board has one user RGB LED:

=======================  =====  =====  ===================================
Devicetree node          Color  Pin    Pin Functions
=======================  =====  =====  ===================================
led0 / user_led1_red     Red    PTE14  FXIO D7 / EMIOS0 CH19
led1 / user_led1_green   Green  PTA27  FXIO D5 / EMIOS1 CH10 / EMIOS2 CH10
led2 / user_led1_blue    Blue   PTE12  FXIO D8 / EMIOS1 CH5
=======================  =====  =====  ===================================

The user can control the LEDs in any way. An output of ``0`` illuminates the LED.

Buttons
-------

The MR-CANHUBK3 board has two user buttons:

=======================  =====  =====  ==============
Devicetree node          Label  Pin    Pin Functions
=======================  =====  =====  ==============
sw0 / user_button_1      SW1    PTD15  EIRQ31
sw0 / user_button_2      SW2    PTA25  EIRQ5 / WKPU34
=======================  =====  =====  ==============

System Clock
============

The Arm Cortex-M7 (Lock-Step) are configured to run at 160 MHz.

Serial Console
==============

By default, the serial console is provided through ``lpuart2`` on the 7-pin
DCD-LZ debug connector ``P6``.

=========  =====  ============
Connector  Pin    Pin Function
=========  =====  ============
P6.2       PTA9   LPUART2_TX
P6.3       PTA8   LPUART2_RX
=========  =====  ============

FS26 SBC Watchdog
=================

On normal operation after the board is powered on, there is a window of 256 ms
on which the FS26 watchdog must be serviced with a good token refresh, otherwise
the watchdog will signal a reset to the MCU. Currently there is no driver for
the watchdog so the FS26 must be started in debug mode following these steps:

1. Power off the board.
2. Remove the jumper ``JP1`` (pins 1-2 open), which is connected by default.
3. Power on the board.
4. Reconnect the jumper ``JP1`` (pins 1-2 shorted).

Programming and Debugging
*************************

Applications for the ``mr_canhubk3`` board can be built in the usual way as
documented in :ref:`build_an_application`.

This board configuration supports `Lauterbach TRACE32`_ and `SEGGER J-Link`_
West runners for flashing and debugging applications. Follow the steps described
in :ref:`lauterbach-trace32-debug-host-tools` and :ref:`jlink-debug-host-tools`,
to setup the flash and debug host tools for these runners, respectively. The
default runner is Lauterbach TRACE32.

Flashing
========

Run the ``west flash`` command to flash the application to the board using
Lauterbach TRACE32. Alternatively, run ``west flash -r jlink`` to use SEGGER
J-Link.

The Lauterbach TRACE32 runner supports additional options that can be passed
through command line:

.. code-block:: console

   west flash --startup-args elfFile=<elf_path> loadTo=<flash/sram>
      eraseFlash=<yes/no> verifyFlash=<yes/no>

Where:

- ``<elf_path>`` is the path to the Zephyr application ELF in the output
  directory
- ``loadTo=flash`` loads the application to the SoC internal program flash
  (:kconfig:option:`CONFIG_XIP` must be set), and ``loadTo=sram`` load the
  application to SRAM. Default is ``flash``.
- ``eraseFlash=yes`` erases the whole content of SoC internal flash before the
  application is downloaded to either Flash or SRAM. This routine takes time to
  execute. Default is ``no``.
- ``verifyFlash=yes`` verify the SoC internal flash content after programming
  (use together with ``loadTo=flash``). Default is ``no``.

For example, to erase and verify flash content:

.. code-block:: console

   west flash --startup-args elfFile=build/zephyr/zephyr.elf loadTo=flash eraseFlash=yes verifyFlash=yes

Debugging
=========

Run the ``west debug`` command to launch the Lauterbach TRACE32 software
debugging interface. Alternatively, run ``west debug -r jlink`` to start a
command line debugging session using SEGGER J-Link.

References
**********

.. target-notes::

.. _NXP MR-CANHUBK3:
   https://www.nxp.com/design/development-boards/automotive-development-platforms/s32k-mcu-platforms/s32k344-evaluation-board-for-mobile-robotics-incorporating-100baset1-and-six-can-fd:MR-CANHUBK344

.. _NXP S32K344:
   https://www.nxp.com/products/processors-and-microcontrollers/s32-automotive-platform/s32k-auto-general-purpose-mcus/s32k3-microcontrollers-for-automotive-general-purpose:S32K3

.. _NXP FS26 Safety System Basis Chip:
   https://www.nxp.com/products/power-management/pmics-and-sbcs/safety-sbcs/safety-system-basis-chip-with-low-power-fit-for-asil-d:FS26

.. _Lauterbach TRACE32:
   https://www.lauterbach.com

.. _SEGGER J-Link:
   https://wiki.segger.com/NXP_S32K3xx
