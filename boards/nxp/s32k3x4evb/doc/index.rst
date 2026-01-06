.. zephyr:board:: s32k3x4evb_t172

Overview
********

The `NXP S32K3X4EVB-T172`_ is an evaluation and development board based on the
`NXP S32K3 Series`_ microcontroller with Arm® Cortex-M7® core (Lock-Step). It
is intended for general-purpose industrial and automotive applications and
includes built-in safety and security features. The board uses an Arduino
UNO-compatible pin layout, enabling easy expansion and rapid prototyping.

This development kit has the following Interfaces:

* :abbr:`ADC (Analog to Digital Converter)`
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`QSPI (Quad Serial Peripheral Interface)`
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`LIN (Local Interconnect Network)`
* :abbr:`ETH (Ethernet MII/RMII 100 Ethernet Phy)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`CAN (Controller Area Network)`

Board Revisions
----------------

This Board have Revisions:
``A``, ``B``, ``B1`` and ``B2`` (Rev B1 and B2 have same configuration)
RevB have the possibility to change the S32K344 for the S32K358, so some
features of the K358 was added both the footprint to populate a SD/MMC socket
and the PGOOD connection between the FS26 and the K358.
All ``TRACE`` signals are ``DISABLED`` as default configuration in ``S32K3X4EVB-T172 RevB``.
In order to enable the TRACE interface, the MCU signals routed to the
QSPIA(R451,R189,R447,R512) and Ethernet interface(R2274, R2283,R2284,R2285,R2286)
must be disabled and isolated, but the TRACE resistors (R192,RR452,R190,R435,R511)
must be populated.

The following table summarizes the differences between the board revisions:
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------------+
|| Interface /   || Signal / || Rev A Config   || Rev B Config   || Rev B1 Config  || Description / Comment                      |
+================+===========+=================+=================+=================+=============================================+
|| Ethernet      || Ethernet || MII Enabled    || RMII Enabled   || RMII Enabled   || TJA1103A Ethernet PHY, 100 Mbit/s          |
||               || PHY      ||                ||                ||                ||                                            |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------------+
| QSPI-A Memory  | U16       | Enabled         | Disabled        | Disabled        | QSPI-A interface disabled on Rev B/B1       |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------------+
|| TRACE         || J12      || Enabled        || Disabled       || Disabled       || TRACE disabled by default on 20-pin Cortex |
||               ||          ||                ||                ||                || Debug ETM                                  |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------------+
|| CAN Interface || CAN0 RX  || TJA1153 / PTA6 || TJA1153 / PTA6 || TJA1443 / PTA6 || CAN0_RX routed to PTA26 (PHY varies by     |
||               ||          ||                ||                ||                || revision)                                  |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------------+
|                | CAN0 TX   | TJA1153 / PTA7  | TJA1153 / PTA7  | TJA1443 / PTA7  | CAN0_TX routed to PTA27                     |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------------+
|                | CAN0 ERRN | TJA1153 / NC    | TJA1153 / NC    | TJA1443 / PTC23 | CAN error signal                            |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------------+
|                | CAN EN    | TJA1153 / PTC21 | TJA1153 / PTC21 | TJA1443 / PTC21 | Transceiver ENABLE                          |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------------+
|                | CAN STB   | TJA1153 / PTC20 | TJA1153 / PTC20 | TJA1443 / PTC20 | Transceiver STANDBY                         |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------------+


Hardware
********

The S32K3X4EVB-T172 evaluation board has 16MHz external crystal oscillator
for the main system clock. The board required

- NXP S32K344

  - Arm Cortex-M7 (Lock-Step), 160 MHz (Max.)
  - 4 MB of program flash, with ECC
  - 320 KB RAM, with ECC
  - Ethernet 100 Mbps, CAN FD, FlexIO, QSPI
  - 12-bit 1 Msps ADC, 16-bit eMIOS timer

- Interfaces

   - TJA1103: Automotive Ethernet Phy 100BASE-T1 MII/RMII mode, ASIL B
   - TJA1153/TJA1443: CAN Transceiver Phy
   - MX25L6433F: QSPI Flash (64M-bit)
   - Arduino UNO R3 compatible headers
   - User RGB LED
   - User Buttons (2 Nos)
   - JTAG/SWD Connector
   - 20-Pin Cortex Debug + ETM Connector

- `NXP FS26 Safety System Basis Chip`_

More information about the hardware and design resources can be found at
`NXP S32K3X4EVB-T172`_ website.

Supported Features
==================

.. zephyr:board-supported-hw::

See `NXP S32K3X4EVB-T172`_
for a complete list of hardware features.

Connections and IOs
===================

Each GPIO port is divided into two banks: low bank, from pin 0 to 15, and high
bank, from pin 16 to 31. For example, ``PTA2`` is the pin 2 of ``gpioa_l`` (low
bank), and ``PTA20`` is the pin 4 of ``gpioa_h`` (high bank).

The GPIO controller provides the option to route external input pad interrupts
to either the SIUL2 EIRQ or WKPU interrupt controllers, as supported by the SoC.
By default, GPIO interrupts are routed to SIUL2 EIRQ interrupt controller,
unless they are explicitly configured to be directed to the WKPU interrupt
controller, as outlined in :zephyr_file:`dts/bindings/gpio/nxp,siul2-gpio.yaml`.

The table below summarizes the mapping between the Port columns in the RM and Zephyr’s ports and pins.
Please consider this mapping when using the GPIO driver or configuring pin multiplexing for device drivers.

+-------------------+--------------------+
|  Ports in S32k344 | Zephyr Ports/Pins  |
+===================+====================+
| GPIO0 - GPIO31    | PA0 - PA31         |
+-------------------+--------------------+
| GPIO32 - GPIO62   | PB0 - PB30         |
+-------------------+--------------------+
| GPIO64 - GPIO95   | PC0 - PC31         |
+-------------------+--------------------+
| GPIO96 - GPIO127  | PD0 - PD31         |
+-------------------+--------------------+
| GPIO128 - GPIO154 | PE0 - PE26         |
+-------------------+--------------------+


LEDs
----

The MR-CANHUBK3 board has one user RGB LED:

=======================  =====  ===============    ================================================
Devicetree node          Color  Pin                Pin Function
=======================  =====  ===============    ================================================
led0 / user_led_red      Red    PTA29              GPIO 29 / EMIOS_1 CH12 / EMIOS_2 CH12
led1 / user_led_green    Green  PTA30              GPIO 30 / EMIOS_1 CH11 / EMIOS_2 CH11 / EIRQ6
led2 / user_led_blue     Blue   PTA31              GPIO 31 / EMIOS_1 CH14 /
=======================  =====  ===============    ================================================

The user can control the LEDs in any way. An output of ``1`` illuminates the LED.

Buttons
-------

There are 2 push-buttons active high (pulled low), the push
button switches connected to MCU ports. The switches are
connected as follows:

=======================  =======  ==============   =========================
Devicetree node          Label    Pin              Pin Function
=======================  =======  ==============   =========================
sw0 / user_button_0      USERSW0  PTB26            GPIO58 / EIRQ13 / WKPU41
sw1 / user_button_1      USERSW1  PTB19            GPIO82 / WKPU38
=======================  =======  ==============   =========================

System Clock
============

- The Arm Cortex-M7 (Lock-Step) are configured to run at 160 MHz.


Serial Console
==============

The EVB includes an on-board debugger that provides a serial console through ``lpuart6`` to the host via USB.
jumper ``J44`` can be used to disconnect the LPUART6 from the OpenSDA debugger and connect it to an external
device for serial communication.

=========  =====  ============
Connector  Pin    Pin Function
=========  =====  ============
J44.1      PTA15   LPUART6_TX
J44.2      PTA16   LPUART6_RX
=========  =====  ============


CAN
===

CAN interface is available through ``J32`` interface Pin 1 CAN_H and Pin 2 CAN_L
CAN2 is also connected to the CAN transceiver on the on-board.

===============  =======  ===============  =============
Devicetree node  Pin      Pin Function     Bus Connector
===============  =======  ===============  =============
flexcan0         PTA6     PTA6_CAN0_RX     J32.2 (CAN_L)
                 PTA7     PTA7_CAN0_TX     J32.1 (CAN_H)

.. note::
    Revision A and B of the S32K3X4EVB-T172 is using TJA1153 CAN transceive while Revision B1 and B2 is using TJA1443 CAN transceiver. The only difference is ERROR pin in the TJA1443, which is NC In TJA1153.

ADC
===

A ADC Rotary Potentiometer, which routes a voltage between 0v
to VD_HV_A is connected to ADC Precise Input Chanel as follows:

====================  ========  ==============  =============
Devicetree node       Label     Pin             Pin Function
====================  ========  ==============  =============
adc0 / user_adc_0     ADC_POT0  PTA11 : GPIO11   ADC1_S10
====================  ========  ==============  =============


Lin Phy Interface
======================
The board includes two LIN interface connected to mcu using LIN transceiver
TJA1021T/20/C, supporting both master and slave mode using jumper. The output from
the LIN transceiver is connected to ``J23``.

=========      =====  ============
Connector      Pin    Pin Function
=========      =====  ============
J23.8 (LIN1)   PTB09   LPUART9_TX
               PTB10   LPUART9_RX
J43.7 (LIN2)   PTB28   LPUART5_RX
               PTB27   LPUART5_TX
=========      =====  ============

Ethernet
========

This board has a single instance of Ethernet Media Access Controller (EMAC)
interfacing with a `NXP TJA1103`_ 100Base-T1 Ethernet PHY. The output from
the PHY is connected to ``J428.1`` as TX and ``J428.2`` as RX connpinector.

================  =========    ================
Devicetree node    Pin          Pin Function
================  =========    ================
emac0              PTB05       MII_RMII_MDC
                   PTB04       MII_RMII_MDIO
                   PTC02       MII_RMII_TXD0
                   PTD07       MII_RMII_TXD1
                   PTD06       MII_TXD2
                   PTD05       MII_TXD3
                   PTD12       MII_RMII_TX_EN
                   PTD11       MII_RMII_TX_CLK
                   PTD10       MII_RX_CLK
                   PTC01       MII_RMII_RXD0
                   PTC00       MII_RMII_RXD1
                   PTD09       MII_RXD2
                   PTD08       MII_RXD3
                   PTC17       MII_RMII_RX_DV
                   PTC16       MII_RMII_RX_ER
================  =========    ================

.. Note::
   RevA use MII as default and the RevB is stablish to use RMII so the principal difference
   is the pin strapping configuration for the Ethernet PHY and some pins of Rx
   and Tx are connected to fast pins reach the maximum transmission speed.

QSPI Interface
===============
The S32K3X4EVB-T172 incorporates a MX25L6433F is 64Mb bits Serial NOR Flash memory,
which is connected to the QSPI-A Module of the S32K344 MCU.

The QSPI Flash memory is connected as follows:
================  ======      =================
Devicetree node   Pin         Pin Function
================  ======      =================
qspi0             PTD11       QSPI_A_IO0_MCU
                  PTD07       QSPI_A_IO1_MCU
                  PTD12       QSPI_A_IO2_MCU
                  PTC02       QSPI_A_IO3_MCU
                  PTD10       QSPI_A_SCK_MCU
                  PTC03       QSPI_A_CS_MCU
================  ======      =================

It is disabled by default to use the Ethernet RMII interface in all Rev B boards.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``s32k3x4evb-t172`` board can be built in the usual way as
documented in :ref:`build_an_application`.

This board configuration supports `Lauterbach TRACE32`_, `SEGGER J-Link`_ and `pyOCD`_
West runners for flashing and debugging. Follow the steps described in
:ref:`lauterbach-trace32-debug-host-tools`, :ref:`jlink-debug-host-tools` and
:ref:`pyocd-debug-host-tools`, to set up the required host tools.
The default runner is J-Link.

If using TRACE32, ensure you have version >= 2024.09 installed.

Flashing
========

Run the ``west flash`` command to flash the application using SEGGER J-Link.
Alternatively, run ``west flash -r trace32`` to use Lauterbach TRACE32, or
``west flash -r pyocd``` to use pyOCD.

The Lauterbach TRACE32 runner supports additional options that can be passed
through command line:

.. code-block:: console

   west flash -r trace32 --startup-args elfFile=<elf_path> loadTo=<flash/sram>
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

   west flash -r trace32 --startup-args elfFile=build/zephyr/zephyr.elf loadTo=flash eraseFlash=yes verifyFlash=yes


Debugging
=========
Run the ``west debug`` command to start a GDB session using SEGGER J-Link.
Alternatively, run ``west debug -r trace32`` or ``west debug -r pyocd``
to launch the Lauterbach TRACE32 or pyOCD software debugging interface respectively.

.. include:: ../../common/board-footer.rst.inc

References
**********

.. target-notes::

.. _NXP S32K3X4EVB-T172:
   https://www.nxp.com/design/design-center/development-boards-and-designs/S32K3X4EVB-T172

.. _NXP S32K3 Series:
   https://www.nxp.com/products/S32K3

.. _NXP FS26 Safety System Basis Chip:
   https://www.nxp.com/products/power-management/pmics-and-sbcs/safety-sbcs/safety-system-basis-chip-with-low-power-fit-for-asil-d:FS26

.. _NXP TJA1103:
   https://www.nxp.com/products/interfaces/ethernet-/automotive-ethernet-phys/asil-b-compliant-100base-t1-ethernet-phy:TJA1103

.. _RDDRONE-T1ADAPT:
   https://www.nxp.com/products/interfaces/ethernet-/automotive-ethernet-phys/ethernet-media-converter-for-drones-rovers-mobile-robotics-and-automotive:RDDRONE-T1ADAPT

.. _Lauterbach TRACE32:
   https://www.lauterbach.com

.. _SEGGER J-Link:
   https://wiki.segger.com/NXP_S32K3xx

.. _pyOCD:
   https://pyocd.io/
