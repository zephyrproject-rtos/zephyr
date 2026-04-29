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
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`ETH (Ethernet MII/RMII 100 Ethernet Phy)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`CAN (Controller Area Network)`

Board Revisions
===============

This board has the following hardware revisions:
``A``, ``B``, ``B1`` and ``B2`` (Rev B1 and B2 have same configuration).
The current Zephyr board support targets the ``B2`` revision (RMII ethernet, TJA1443 CAN PHY).

RevB provides the possibility to change the S32K344 for the S32K358, so some
features of the K358 were added, including the footprint to populate a SD/MMC socket
and the PGOOD connection between the FS26 and the K358.

All ``TRACE`` signals are ``DISABLED`` as default configuration in ``S32K3X4EVB-T172 RevB``.
In order to enable the TRACE interface, the MCU signals routed to the
QSPIA (R451, R189, R447, R512) and Ethernet interface (R2274, R2283, R2284, R2285, R2286)
must be disabled and isolated, while the TRACE resistors (R192, R452, R190, R435, R511)
must be populated.

The following table summarizes the differences between the board revisions:

+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------+
| Interface /    | Signal /  | Rev A Config    | Rev B Config    | Rev B1 Config   | Description / Comment                 |
+================+===========+=================+=================+=================+=======================================+
| Ethernet       | Ethernet  | MII Enabled     | RMII Enabled    | RMII Enabled    | TJA1103A Ethernet PHY, 100 Mbit/s     |
|                | PHY       |                 |                 |                 |                                       |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------+
| QSPI-A Memory  | U16       | Enabled         | Disabled        | Disabled        | QSPI-A interface disabled on Rev B/B1 |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------+
| TRACE          | J12       | Enabled         | Disabled        | Disabled        | TRACE disabled by default on 20-pin   |
|                |           |                 |                 |                 | Cortex Debug ETM                      |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------+
| CAN Interface  | CAN0 RX   | TJA1153 / PTA6  | TJA1153 / PTA6  | TJA1443 / PTA6  | CAN0_RX routed to PTA26 (PHY varies)  |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------+
|                | CAN0 TX   | TJA1153 / PTA7  | TJA1153 / PTA7  | TJA1443 / PTA7  | CAN0_TX routed to PTA27               |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------+
|                | CAN0 ERRN | TJA1153 / NC    | TJA1153 / NC    | TJA1443 / PTC23 | CAN error signal                      |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------+
|                | CAN EN    | TJA1153 / PTC21 | TJA1153 / PTC21 | TJA1443 / PTC21 | Transceiver ENABLE                    |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------+
|                | CAN STB   | TJA1153 / PTC20 | TJA1153 / PTC20 | TJA1443 / PTC20 | Transceiver STANDBY                   |
+----------------+-----------+-----------------+-----------------+-----------------+---------------------------------------+

Hardware
********

The S32K3X4EVB-T172 evaluation board has a 16MHz external crystal oscillator
for the main system clock.

- **NXP S32K344**
  - Arm Cortex-M7 (Lock-Step), 160 MHz (Max.)
  - 4 MB of program flash, with ECC
  - 320 KB RAM, with ECC
  - Ethernet 100 Mbps, CAN FD, FlexIO, QSPI
  - 12-bit 1 Msps ADC, 16-bit eMIOS timer

- **Interfaces**
  - TJA1103: Automotive Ethernet Phy 100BASE-T1 MII/RMII mode, ASIL B
  - TJA1153/TJA1443: CAN Transceiver Phy
  - MX25L6433F: QSPI Flash (64M-bit)
  - Arduino UNO R3 compatible headers
  - User RGB LED
  - User Buttons (2 Nos)
  - JTAG/SWD Connector
  - 20-Pin Cortex Debug + ETM Connector

- `NXP FS26 Safety System Basis Chip`_

Supported Features
==================

.. zephyr:board-supported-hw::

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

To find information about which GPIOs are compatible with each interrupt
controller, refer to the device reference manual.

LEDs
----

=======================  =====  ===============  ================================================
Devicetree node          Color  Pin              Pin Function
=======================  =====  ===============  ================================================
led0 / user_led_red      Red    PTA29            GPIO 29 / EMIOS_1 CH12
led1 / user_led_green    Green  PTA30            GPIO 30 / EMIOS_1 CH11 / EIRQ6
led2 / user_led_blue     Blue   PTA31            GPIO 31 / EMIOS_1 CH14
=======================  =====  ===============  ================================================

Buttons
-------

=======================  =======  ==============  =========================
Devicetree node          Label    Pin             Pin Function
=======================  =======  ==============  =========================
sw0 / user_button_0      USERSW0  PTB26           GPIO58 / EIRQ13 / WKPU41
sw1 / user_button_1      USERSW1  PTB19           GPIO82 / WKPU38
=======================  =======  ==============  =========================

System Clock
============

The Arm Cortex-M7 (Lock-Step) is configured to run at 160 MHz.

Serial Console
==============

The EVB includes an on-board debugger that provides a serial console through ``lpuart6``.
jumper ``J44`` can be used to disconnect the LPUART6 from the OpenSDA debugger and connect it to an external
device for serial communication.

=========  =====  ============
Connector  Pin    Pin Function
=========  =====  ============
J44.1      PTA15  LPUART6_TX
J44.2      PTA16  LPUART6_RX
=========  =====  ============

CAN
===

CAN interface is available through ``J32`` (Pin 1: CAN_H, Pin 2: CAN_L).

===============  =======  ===============  =============
Devicetree node  Pin      Pin Function     Bus Connector
===============  =======  ===============  =============
flexcan0         PTA6     PTA6_CAN0_RX     J32.2 (CAN_L)
                 PTA7     PTA7_CAN0_TX     J32.1 (CAN_H)
===============  =======  ===============  =============

.. note::
    Revision A and B use TJA1153 while Revision B1 and B2 use TJA1443.

ADC
===

====================  ========  ==============  =============
Devicetree node       Label     Pin             Pin Function
====================  ========  ==============  =============
adc0 / user_adc_0     ADC_POT0  PTA11           ADC1_S10
====================  ========  ==============  =============

Ethernet
========
This board has a single instance of Ethernet Media Access Controller (EMAC)
interfacing with a `NXP TJA1103`_ 100Base-T1 Ethernet PHY. The output from
the PHY is connected to ``J428.1`` as TX and ``J428.2`` as RX connector.

================  ======  ================
Devicetree node   Pin     Pin Function
================  ======  ================
emac0             PTB05   MII_RMII_MDC
                  PTB04   MII_RMII_MDIO
                  PTC02   MII_RMII_TXD0
                  PTD07   MII_RMII_TXD1
                  PTD12   MII_RMII_TX_EN
                  PTD11   MII_RMII_TX_CLK
                  PTC01   MII_RMII_RXD0
                  PTC00   MII_RMII_RXD1
                  PTC17   MII_RMII_RX_DV
                  PTC16   MII_RMII_RX_ER
================  ======  ================


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``s32k3x4evb-t172`` board can be built in the usual way as
documented in :ref:`build_an_application`.

This board configuration supports `Lauterbach TRACE32`_, `SEGGER J-Link`_, and `pyOCD`_
West runners for flashing and debugging. Follow the steps described in:

* :ref:`lauterbach-trace32-debug-host-tools`
* :ref:`jlink-debug-host-tools`
* :ref:`pyocd-debug-host-tools`

The default runner is J-Link. If using TRACE32, ensure you have version >= 2024.09 installed.

Flashing
========

Follow these steps if you just want to download the application to the board and run.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: s32k3x4evb_t172
   :goals: build flash
   :compact:

Alternatively, run ``west flash -r trace32`` to use Lauterbach TRACE32, or
``west flash -r pyocd`` to use pyOCD.

The Lauterbach TRACE32 runner supports additional options that can be passed
through the command line:

.. code-block:: console

   west flash -r trace32 --startup-args elfFile=build/zephyr/zephyr.elf loadTo=flash

Debugging
=========

You can build and debug the :zephyr:code-sample:`hello_world` sample for the board
``s32k3x4evb_t172`` with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: s32k3x4evb_t172
   :goals: build debug

To debug with Lauterbach TRACE32 software run instead:

.. code-block:: console

   west debug -r trace32

.. include:: ../../common/board-footer.rst.inc

References
**********

.. target-notes::

.. _NXP S32K3X4EVB-T172: https://www.nxp.com/design/design-center/development-boards-and-designs/S32K3X4EVB-T172
.. _NXP S32K3 Series: https://www.nxp.com/products/S32K3
.. _NXP FS26 Safety System Basis Chip: https://www.nxp.com/products/power-management/pmics-and-sbcs/safety-sbcs/safety-system-basis-chip-with-low-power-fit-for-asil-d:FS26
.. _NXP TJA1103: https://www.nxp.com/products/interfaces/ethernet-/automotive-ethernet-phys/asil-b-compliant-100base-t1-ethernet-phy:TJA1103
.. _RDDRONE-T1ADAPT: https://www.nxp.com/products/interfaces/ethernet-/automotive-ethernet-phys/ethernet-media-converter-for-drones-rovers-mobile-robotics-and-automotive:RDDRONE-T1ADAPT
.. _Lauterbach TRACE32: https://www.lauterbach.com
.. _SEGGER J-Link: https://wiki.segger.com/NXP_S32K3xx
.. _pyOCD: https://pyocd.io/
