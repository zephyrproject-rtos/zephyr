.. zephyr:board:: frdm_imxrt1186

.. _frdm_imxrt1186:

FRDM-iMXRT1186
##############

Overview
********

NXP FRDM-IMXRT1186 is a cost-effective development board based on NXP's i.MX RT1186 crossover MCU,
combining high performance with real-time capabilities. Easy I/O access supports expansion boards
for fast prototyping and rapid evaluation of Programmable Logic Controllers, Remote IOs, motor
control and other industrial automation applications.

Hardware
********

- i.MX RT1186 Crossover Processor:

  - 800 MHz Arm Cortex-M7 core
  - 300 MHz Arm Cortex-M33 core
  - First real-time microcontroller (MCU) with an integrated Gbps time-sensitive network (TSN)
    switch supporting multiple protocols bridging communications between existing systems and
    Industry 4.0
  - First crossover MCU with an integrated EdgeLock® Secure Enclave
  - Up to 1.5 MB SRAM (ECC protected) with 512 KB of TCM for Arm Cortex-M7 and 256 KB of TCM for
    Arm Cortex-M33

- Board Memory

  - 256 Mbit HyperRAM memory
  - 512 Mbit HyperFlash
  - 128 Mbit QSPI Flash

- Connectivity

  - 2x Ethernet (10/100/1000M) connectors for TSN switch
  - 2x Ethernet (10/100M) connectors for EtherCAT or TSN switch
  - 1x Type-C USB OTG connector
  - 1x CAN transceivers

- Debug Capabilities

  - JTAG connector
  - On-board MCU-Link debugger

- Expansion Connectors

  - ARDUINO® UNO R3 header
  - FRDM header
  - FlexSPI Follower header
  - SRAMC header
  - Motor header

For more information about the i.MX RT1186 SoC and FRDM-iMXRT1186 board, see:

- `i.MX RT1180 Website`_
- `i.MX RT1180 Datasheet`_
- `i.MX RT1180 Reference Manual`_
- `FRDM-iMXRT1186 Website`_
- `FRDM-iMXRT1186 User Guide`_
- `FRDM-iMXRT1186 Schematics`_

Board Variants
==============

This board has three variants available:

- ``frdm_imxrt1186/mimxrt1186/cm33``: Runs on the Cortex-M33 core, using
  internal SRAM and Flash. This is the default bootable core.

- ``frdm_imxrt1186/mimxrt1186/cm7``: Runs on the Cortex-M7 core, using
  internal SRAM and Flash. Must be started by the CM33 core.

- ``frdm_imxrt1186/mimxrt1186/cm7/extmem``: Runs on the Cortex-M7 core,
  using external HyperRAM for ``zephyr,sram`` and external HyperFlash for
  ``zephyr,flash``. This variant is provided for applications that require
  more RAM or Flash than available in the internal memory. ITCM and DTCM
  remain available for time-critical code and data.

.. note::
   When using the ``extmem`` variant, external memory is initialized through
   J-Link debug scripts to ensure proper configuration before use.

Supported Features
==================

The FRDM-iMXRT1186 board is a cost-effective development platform. NXP considers
the MIMXRT1180-EVK as the superset board for the i.MX RT118x family of MCUs.
The MIMXRT1180-EVK is the focus for NXP's Full Platform Support for Zephyr,
to better enable the entire RT118x family. NXP prioritizes enabling the
MIMXRT1180-EVK with new support for Zephyr features. Some features may be
limited or unavailable on the FRDM-iMXRT1186 compared to the MIMXRT1180-EVK.

.. zephyr:board-supported-hw::

Connections and I/Os
====================

The i.MX RT1186 SoC has multiple GPIO controllers. The following pins are used
by the board for the default configuration:

+---------------+-----------------+---------------------------+
| Name          | Function        | Usage                     |
+===============+=================+===========================+
| GPIO_AON_08   | LPUART1_TX      | UART Console TX (CM33)    |
+---------------+-----------------+---------------------------+
| GPIO_AON_09   | LPUART1_RX      | UART Console RX (CM33)    |
+---------------+-----------------+---------------------------+
| GPIO_AD_13    | LPUART3_TX      | UART Console TX (CM7)     |
+---------------+-----------------+---------------------------+
| GPIO_AD_14    | LPUART3_RX      | UART Console RX (CM7)     |
+---------------+-----------------+---------------------------+
| GPIO2_IO09    | GPIO            | Red LED                   |
+---------------+-----------------+---------------------------+
| GPIO2_IO11    | GPIO            | Green LED                 |
+---------------+-----------------+---------------------------+
| GPIO3_IO07    | GPIO            | Blue LED                  |
+---------------+-----------------+---------------------------+
| GPIO4_IO12    | GPIO            | User Button SW4           |
+---------------+-----------------+---------------------------+
| GPIO_AD_15    | ADC             | ADC1 Channel 1            |
+---------------+-----------------+---------------------------+
| GPIO_AD_16    | ADC             | ADC1 Channel 0            |
+---------------+-----------------+---------------------------+

System Clock
============

The i.MX RT1186 SoC is configured to use SysTick as the system clock source.
The Cortex-M33 core can run up to 300 MHz, and the Cortex-M7 core can run up
to 800 MHz.

ELE Active Timer Requirement
=============================

The RT1180 platform requires periodic communication with the EdgeLock Enclave (ELE)
to prevent system reset. According to the RT1180 System Reference Manual (SRM) section
3.11 "ELE active timer", the ELE must be pinged at least once every 24 hours.

Zephyr implements this requirement using a software timer that automatically pings the
ELE every 23 hours (instead of 24 hours) to account for potential clock inaccuracies.
This is transparent to the application and requires no user intervention.

For more details, refer to the RT1180 SRM section 3.11.

ITCM and DTCM
=============

If placing ``zephyr,flash`` in ITCM or ``zephyr,sram`` in DTCM, the property
``zephyr,memory-region`` should be deleted from the memory device node.
For example, this overlay moves the CM33 ``zephyr,sram`` to DTCM:

.. code-block:: none

   boards/nxp/frdm_imxrt1186/cm33_sram_dtcm.overlay

Ethernet
========

NETC Ethernet driver supports to manage the Physical Station Interface (PSI).
NETC DSA driver supports to manage switch ports. Current DSA support is with
limitation that only switch function is available without management via
DSA master port. DSA master port support is TODO work.

For FRDM-iMXRT1186, the following network interfaces present:

- ``swp0`` and ``swp2``: user ports which can be used.
- ``swp4``: DSA CPU port. Not a user port.
- ``eth0``: DSA conduit port. Not a user port.

.. note::

   DHCP is expected to work on ``swp0`` and ``swp2``.

.. code-block:: none

                   +--------+                  +--------+
                   | ENETC1 |                  | ENETC0 |
                   |        |                  |        |
                   | Pseudo |                  |  1G    |
                   |  MAC   |                  |  MAC   |
                   +--------+                  +--------+
                       | zero copy interface       |
   +-------------- +--------+----------------+     |
   |               | Pseudo |                |     |
   |               |  MAC   |                |     |
   |               |        |                |     |
   |               | Port 4 |                |     |
   |               +--------+                |     |
   |           SWITCH       CORE             |     |
   +--------+ +--------+ +--------+ +--------+     |
   | Port 0 | | Port 1 | | Port 2 | | Port 3 |     |
   |        | |        | |        | |        |     |
   |  1G    | |  1G    | |  1G    | |  1G    |     |
   |  MAC   | |  MAC   | |  MAC   | |  MAC   |     |
   +--------+-+--------+-+--------+-+--------+     |
       |          |          |          |          |
   NETC External Interfaces (4 switch ports, 1 end-point port)

Serial Port
===========

The i.MX RT1186 SoC has 8 LPUART peripherals. LPUART1/3 is configured for the
console and is available on the Debug USB connector.

Programming and Debugging
**************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
==========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the :ref:`mcu-link-cmsis-onboard-debug-probe`.

When debugging the CM33 core, ensure jumper **J60** is set to **1:OFF 2:OFF 3:ON**.

When debugging the CM7 core, ensure jumper **J60** is set to **1:ON 2:OFF 3:OFF**.

Using LinkServer
----------------

LinkServer is the default debug host tool for this board and requires no
additional configuration.

Using J-Link
------------

Install the :ref:`jlink-debug-host-tools` and make sure they are in your search
path.

There are two options:

1. Short J40 and update the onboard MCU-Link debug circuit with Segger J-Link firmware.
2. Short J58 and use an :ref:`jlink-external-debug-probe` connected to J54
   (note: J54 is not populated by default on the board).

Configuring a Console
=====================

Connect a USB Type-C cable from your PC to J23, and use the serial terminal of your
choice(minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application for the CM33 core.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imxrt1186/mimxrt1186/cm33
   :goals: flash

Open a serial terminal, reset the board (press the RESET button SW2), and you should
see the following message in the terminal:
(Note: When using LPUART3 as serial port, jumper J34 must be removed)

.. code-block:: console

   *** Booting Zephyr OS build v4.x.x ***
   Hello World! frdm_imxrt1186/mimxrt1186/cm33

Note: By default, the CM33 core is the bootable core. The CM7 core cannot be directly
      booted and must be started by the CM33 core.

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imxrt1186/mimxrt1186/cm33
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.x.x ***
   Hello World! frdm_imxrt1186/mimxrt1186/cm33

.. include:: ../../common/board-footer.rst.inc

.. _i.MX RT1180 Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i-mx-rt-crossover-mcus/i-mx-rt1180-crossover-mcu-dual-core-arm-cortex-m7-and-cortex-m33-with-tsn-switch:i.MX-RT1180

.. _i.MX RT1180 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/IMXRT1180CEC.pdf

.. _i.MX RT1180 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMXRT1180RM

.. _FRDM-iMXRT1186 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-IMXRT1186#on-board-devices

.. _FRDM-iMXRT1186 User Guide:
   https://www.nxp.com/webapp/Download?colCode=UM12450

.. _FRDM-iMXRT1186 Schematics:
   https://www.nxp.com/webapp/Download?colCode=FRDM-IMXRT1186-DESIGN-FILES
