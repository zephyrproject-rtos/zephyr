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

Supported Features
==================

The ``frdm_imxrt1186/mimxrt1186/cm33`` and ``frdm_imxrt1186/mimxrt1186/cm7``
board configurations support the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig files:
:zephyr_file:`boards/nxp/frdm_imxrt1186/frdm_imxrt1186_mimxrt1186_cm33_defconfig`
:zephyr_file:`boards/nxp/frdm_imxrt1186/frdm_imxrt1186_mimxrt1186_cm7_defconfig`

Other hardware features are not currently supported by the port.

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

System Clock
============

The i.MX RT1186 SoC is configured to use SysTick as the system clock source.
The Cortex-M33 core can run up to 300 MHz, and the Cortex-M7 core can run up
to 800 MHz.

Serial Port
===========

The i.MX RT1186 SoC has 8 LPUART peripherals. LPUART1/3 is configured for the
console and is available on the Debug USB connector.

Programming and Debugging
**************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

When debugging cm33 core, need to ensure the J60 on "001" mode.
When debugging cm7 core, need to ensure the J60 on "100" mode.

Configuring a Debug Probe
==========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the on-board MCU-Link Microcontroller.

Using J-Link
------------

Install the :ref:`jlink-debug-host-tools` and make sure they are in your search
path.

There are two options: the onboard debug circuit can be updated with Segger
J-Link firmware, or :ref:`jlink-external-debug-probe` can be attached to the
JTAG header J62(Also need to short J58 if using J62 to debug).

In either case, use the ``-r jlink`` option with west to use the J-Link debug
probe:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imxrt1186/mimxrt1186/cm33
   :goals: flash

Configuring a Console
=====================

Connect a USB cable from your PC to J23, and use the serial terminal of your choice
(minicom, putty, etc.) with the following settings:

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

Open a serial terminal, reset the board (press the RESET button), and you should
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
