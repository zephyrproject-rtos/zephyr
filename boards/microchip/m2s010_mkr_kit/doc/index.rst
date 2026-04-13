.. SPDX-FileCopyrightText: Copyright Bavariamatic GmbH
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: m2s010_mkr_kit

Overview
********

The Microchip M2S010-MKR-KIT board is a SmartFusion2 FPGA development kit based
on the Microchip M2S010 device with an integrated ARM Cortex-M3 subsystem.
The Zephyr board target for this port is ``m2s010_mkr_kit/m2s010``.

Reference material for the kit is available in the
`M2S010-MKR-KIT Quick Start Guide <https://ww1.microchip.com/downloads/aemdocuments/documents/fpga/ProductDocuments/UserGuides/microsemi_digikey_quick_start_guide_120x180mm.pdf>`_.

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Applications for the ``m2s010_mkr_kit/m2s010`` board target can be built as
usual:

.. zephyr-app-commands::
   :board: m2s010_mkr_kit/m2s010
   :goals: build

Clock Configuration
===================

The current SmartFusion2 port does not program the MSS clock tree from Zephyr.
The actual CPU and peripheral clocks must already be configured in the
SmartFusion2 hardware design, for example in the MSS/Libero configuration used
to build the board image.

In Zephyr, the software-visible CPU frequency is taken from the devicetree CPU
node:

- ``&cpu0 { clock-frequency = <...>; }``

That value is then used to derive:

- ``CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC``
- the ``SystemCoreClock`` variable used by the SmartFusion2 SoC port

The SmartFusion2-specific clock-controller node is currently used for UART
clocking, while the remaining peripherals still use their local devicetree
frequency properties. If the hardware clock configuration changes, update the
devicetree to match the real board configuration. For example, an application
overlay can override the CPU and peripheral clock description values:

.. code-block:: dts

   &cpu0 {
      clock-frequency = <80000000>;
   };

   &clkc {
      clock-frequencies = <80000000 40000000>;
   };

The devicetree values must describe the real hardware clocks. Changing them in
Zephyr alone does not reprogram the SmartFusion2 clock hardware.

Flashing
========

The board uses the OpenOCD runner configuration from ``board.cmake`` and
``support/openocd.cfg``. Once the CMSIS-DAP/OpenOCD setup for the kit is
available in the host environment, the usual commands are:

.. code-block:: bash

   west flash --runner openocd
   west debug --runner openocd

Helpful Documentation
********************

The following Microchip documents are useful when extending or upstreaming the
board support.

Board Documentation
===================

- `M2S010-MKR-KIT Quick Start Guide <https://ww1.microchip.com/downloads/aemdocuments/documents/fpga/ProductDocuments/UserGuides/microsemi_digikey_quick_start_guide_120x180mm.pdf>`_

SoC Documentation
=================

- `IGLOO 2 FPGA and SmartFusion 2 SoC FPGA Datasheet (DS00004750) <https://ww1.microchip.com/downloads/aemDocuments/documents/FPGA/ProductDocuments/DataSheets/IGLOO-2-FPGA-And-SmartFusion-2-SoC-FPGA-Data-Sheet-DS00004750.pdf>`_
- `SmartFusion2 and IGLOO2 Programming User Guide (UG0451) <https://ww1.microchip.com/downloads/aemdocuments/documents/FPGA/ProductDocuments/SoC/microsemi_smartfusion2_igloo2_programming_user_guide_ug0451_v9.pdf>`_

The quick start guide is useful for board bring-up, connector overview and kit
contents. The device datasheet is the better top-level reference for package
options, memory sizes, hard IP inventory and electrical capabilities. The
programming guide is the better reference for flash programming, device
configuration flows, boot/programming modes and debug-related setup.

Hardware
********

The current board support package models the following verified platform blocks:

- ARM Cortex-M3 CPU
- NVIC interrupt controller
- 256 KB embedded non-volatile memory used for code storage
- 64 KB SRAM
- MSS UART0 as Zephyr console
- MSS GPIO with one user LED on GPIO0 pin 0

The devicetree also enumerates additional SmartFusion2 peripheral blocks so they
can be enabled incrementally as Zephyr drivers are upstreamed:

- SPI0 and SPI1
- I2C0 and I2C1
- PDMA
- timers and watchdog
- CAN and RTC

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| UART      | MSS UART0  | ns16550                             |
+-----------+------------+-------------------------------------+
| GPIO      | MSS GPIO   | gpio                                |
+-----------+------------+-------------------------------------+
| FLASH     | eNVM       | soc-nv-flash                        |
+-----------+------------+-------------------------------------+

Other on-chip peripherals are described in devicetree but remain disabled until
matching Zephyr drivers are available and validated on the board.

Pin and Interface Mapping
*************************

The table below summarizes the exact board-level wiring recovered from the
``DIGIKEY MAKER KIT REVA1_0_20170606.pdf`` schematic bundled with the kit.
Signal names use the schematic net names and SmartFusion2 package ball names.

User LEDs and Buttons
=====================

+----------------+--------------------------+-------------------------------+
| Board function | SmartFusion2 signal      | Notes                         |
+================+==========================+===============================+
| ``led0``       | ``DDRIO47PB0``           | Exposed in Zephyr as LED0     |
+----------------+--------------------------+-------------------------------+
| LED1           | ``DDRIO47NB0``           | On-board red LED              |
+----------------+--------------------------+-------------------------------+
| LED2 / LED3    | ``DDRIO44PB0`` and       | Middle pair in the 8-LED bank |
|                | ``DDRIO44NB0``           |                               |
+----------------+--------------------------+-------------------------------+
| LED4           | ``DDRIO43PB0``           | On-board red LED              |
+----------------+--------------------------+-------------------------------+
| LED5           | ``DDRIO43NB0``           | On-board red LED              |
+----------------+--------------------------+-------------------------------+
| LED6           | ``DDRIO37PB0``           | On-board red LED              |
+----------------+--------------------------+-------------------------------+
| LED7           | ``DDRIO37NB0``           | On-board red LED              |
+----------------+--------------------------+-------------------------------+
| USER_PB1       | ``DDRIO59PB0/GB0``       | User pushbutton SW1           |
+----------------+--------------------------+-------------------------------+
| USER_PB2       | ``DDRIO63PB0``           | User pushbutton SW2           |
+----------------+--------------------------+-------------------------------+
| ``SF2_RST_N``  | dedicated reset net      | Reset pushbutton SW3          |
+----------------+--------------------------+-------------------------------+

The schematic labels the discrete LEDs as D2 through D9. The currently enabled
Zephyr LED maps to the ``LED0`` net, which is routed to ``DDRIO47PB0``.

Debug, Clock and Console Routing
================================

+-------------------+-----------------------------------+-------------------------+
| Function          | SmartFusion2 signal              | External device          |
+===================+===================================+=========================+
| Main oscillator   | ``XTLOSC_MAIN_XTAL/EXTAL``       | 32.768 kHz crystal      |
+-------------------+-----------------------------------+-------------------------+
| Auxiliary osc.    | ``XTLOSC_AUX_XTAL/EXTAL``        | DNI on the schematic    |
+-------------------+-----------------------------------+-------------------------+
| JTAG select       | ``JTAGSEL``                      | Header/jumper network   |
+-------------------+-----------------------------------+-------------------------+
| JTAG TDO          | ``JTAG_TDO/M3_TDO/M3_SWO``       | FT4232H JTAG bridge     |
+-------------------+-----------------------------------+-------------------------+
| JTAG TCK          | ``JTAG_TCK/M3_TCK``              | FT4232H JTAG bridge     |
+-------------------+-----------------------------------+-------------------------+
| JTAG TRST         | ``JTAG_TRSTB/M3_TRSTB``          | FT4232H JTAG bridge     |
+-------------------+-----------------------------------+-------------------------+
| JTAG TMS / SWDIO  | ``JTAG_TMS/M3_TMS/M3_SWDIO``     | FT4232H JTAG bridge     |
+-------------------+-----------------------------------+-------------------------+
| JTAG TDI          | ``JTAG_TDI/M3_TDI``              | FT4232H JTAG bridge     |
+-------------------+-----------------------------------+-------------------------+
| MSS UART TX       | ``UART_TXD_OUT``                 | FT4232H USB UART        |
+-------------------+-----------------------------------+-------------------------+
| MSS UART RX       | ``UART_RXD_IN``                  | FT4232H USB UART        |
+-------------------+-----------------------------------+-------------------------+
| USB UART TX net   | ``USB_UART_TXD``                 | Routed to FT4232H       |
+-------------------+-----------------------------------+-------------------------+
| USB UART RX net   | ``USB_UART_RXD``                 | Routed to FT4232H       |
+-------------------+-----------------------------------+-------------------------+

On-board SPI Flash and Light Sensor
===================================

+---------------------+----------------------------------+-------------------------+
| Peripheral signal   | SmartFusion2 signal              | Connected device        |
+=====================+==================================+=========================+
| ``SPI_0_SS0``       | ``MSIO13NB2 / GPIO_7_A``         | AT25SF161 CS            |
+---------------------+----------------------------------+-------------------------+
| ``SPI_0_SDI``       | ``MSIO12NB2 / GPIO_5_A``         | AT25SF161 SI            |
+---------------------+----------------------------------+-------------------------+
| ``SPI_0_SDO``       | ``MSIO13PB2 / GPIO_6_A``         | AT25SF161 SO            |
+---------------------+----------------------------------+-------------------------+
| ``SPI_0_CLK``       | ``MSIO12PB2``                    | AT25SF161 SCK           |
+---------------------+----------------------------------+-------------------------+
| ``SPI_0_WP_N``      | ``MSIO2PB2 / USB_STP_B``         | AT25SF161 WP#           |
+---------------------+----------------------------------+-------------------------+
| ``SPI_0_HOLD_N``    | ``MSIO2NB2 / USB_NXT_B``         | AT25SF161 HOLD#         |
+---------------------+----------------------------------+-------------------------+
| ``LIGHT_SDA``       | ``MSIO4NB2 / USB_DATA3_B``       | LTR-329ALS SDA          |
+---------------------+----------------------------------+-------------------------+
| ``LIGHT_SCL``       | ``MSIO4PB2 / USB_DATA2_B``       | LTR-329ALS SCL          |
+---------------------+----------------------------------+-------------------------+

Optional ESP8266 Footprint
==========================

The schematic contains an optional ESP8266 module footprint marked as not
populated. Its routing is still useful when extending the board support:

+-------------------+----------------------------------+-------------------------+
| ESP8266 signal    | SmartFusion2 signal              | Notes                   |
+===================+==================================+=========================+
| ``MOD1_TX``       | ``MSIO1PB2 / USB_XCLK_B``        | ESP8266 TXO             |
+-------------------+----------------------------------+-------------------------+
| ``MOD1_RX``       | ``MSIO1NB2 / USB_DIR_B``         | ESP8266 RXI             |
+-------------------+----------------------------------+-------------------------+
| ``MOD1_CE``       | ``MSIO3NB2 / USB_DATA1_B``       | ESP8266 CHPD            |
+-------------------+----------------------------------+-------------------------+
| ``MOD1_RST``      | ``MSIO3PB2 / USB_DATA0_B``       | ESP8266 reset           |
+-------------------+----------------------------------+-------------------------+
| ``MOD1_GPIO2``    | ``MSIO0NB2 / USB_DATA7_B``       | ESP8266 GPIO2           |
+-------------------+----------------------------------+-------------------------+
| ``MOD1_GPIO0``    | ``MSIO5NB2 / USB_DATA5_B``       | ESP8266 GPIO0           |
+-------------------+----------------------------------+-------------------------+
| spare routing     | ``MSIO5PB2 / USB_DATA4_B``       | Marked ``BANK2_UNUSED1``|
+-------------------+----------------------------------+-------------------------+
| spare routing     | ``MSIO6PB2 / USB_DATA6_B``       | Marked ``BANK2_UNUSED2``|
+-------------------+----------------------------------+-------------------------+

Optional ESP-WROOM-32 Header
============================

Page 6 of the schematic also exposes an unpopulated ESP-WROOM-32 / WiFi-BT-BLE
header path on bank 7:

+-------------------+-------------------------------+--------------------------+
| Header/net        | SmartFusion2 signal           | Notes                    |
+===================+===============================+==========================+
| ``USB_UART_TXD``  | ``MSIO64NB7``                | Module UART              |
+-------------------+-------------------------------+--------------------------+
| ``USB_UART_RXD``  | ``MSIO64PB7``                | Module UART              |
+-------------------+-------------------------------+--------------------------+
| ``BT_TX``         | ``MSIO67NB7``                | Module serial            |
+-------------------+-------------------------------+--------------------------+
| ``BT_RX``         | ``MSIO67PB7``                | Module serial            |
+-------------------+-------------------------------+--------------------------+
| ``BT_BOOT``       | ``MSIO71NB7``                | Boot strap               |
+-------------------+-------------------------------+--------------------------+
| ``BT_IO34``       | ``MSIO71PB7``                | GPIO                     |
+-------------------+-------------------------------+--------------------------+
| ``BT_IO35``       | ``MSIO72NB7``                | GPIO                     |
+-------------------+-------------------------------+--------------------------+
| ``BT_IO18``       | ``MSIO73NB7``                | GPIO                     |
+-------------------+-------------------------------+--------------------------+
| ``BT_IO22``       | ``MSIO76NB7``                | GPIO                     |
+-------------------+-------------------------------+--------------------------+
| ``BT_IO23``       | ``MSIO77NB7``                | GPIO                     |
+-------------------+-------------------------------+--------------------------+
| ``BT_EN``         | ``MSIO79NB7``                | Enable                   |
+-------------------+-------------------------------+--------------------------+
| ``BT_IO2``        | ``MSIO79PB7/GB1``            | GPIO                     |
+-------------------+-------------------------------+--------------------------+
| J12 pin 1         | ``ESP32_SCS``                | SPI chip select          |
+-------------------+-------------------------------+--------------------------+
| J12 pin 2         | ``ESP32_SCK``                | SPI clock                |
+-------------------+-------------------------------+--------------------------+
| J12 pin 3         | ``ESP32_SDO``                | SPI MISO                 |
+-------------------+-------------------------------+--------------------------+
| J12 pin 4         | ``ESP32_SDI``                | SPI MOSI                 |
+-------------------+-------------------------------+--------------------------+

Ethernet PHY Wiring
===================

The VSC8541 PHY is wired directly to SmartFusion2 fabric banks 4 and 6. The
schematic pages clearly recover the following signal assignments:

+-------------------+-----------------------------------+------------------------+
| PHY/GMII signal   | SmartFusion2 signal               | Bank                   |
+===================+===================================+========================+
| ``CLK_50MHZ_SF2`` | ``MSIOD84PB6/CCC_NE1_CLKI2``      | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_TX_CLK``   | ``MSIOD85PB6/CCC_NE1_CLKI1``      | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_MDIO``     | ``MSIOD90NB6``                    | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_MDC``      | ``MSIOD90PB6``                    | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_TXD7``     | ``MSIOD93NB6``                    | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_TXD6``     | ``MSIOD93PB6``                    | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_TXD5``     | ``MSIOD96PB6``                    | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_TXD4``     | ``MSIOD96NB6``                    | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_TXD3``     | ``MSIOD98NB6``                    | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_TXD2``     | ``MSIOD99PB6``                    | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_TXD1``     | ``MSIOD99NB6``                    | bank 6                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_GTX_CLK``  | ``MSIO102NB4/CCC_NE1_CLKI0``      | bank 4                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_RXD7``     | ``MSIO113NB4``                    | bank 4                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_RXD6``     | ``MSIO113PB4``                    | bank 4                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_RX_DV``    | ``MSIO105NB4``                    | bank 4                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_RX_CLK``   | ``MSIO105PB4/CCC_NE0_CLKI0``      | bank 4                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_RXD1``     | ``MSIO106NB4``                    | bank 4                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_RXD0``     | ``MSIO106PB4``                    | bank 4                 |
+-------------------+-----------------------------------+------------------------+
| ``GMII_RXD3``     | ``MSIO108NB4``                    | bank 4                 |
+-------------------+-----------------------------------+------------------------+

The remaining VSC8541 receive and status nets are present on the same schematic
page, but only the assignments listed above were recoverable with high
confidence from machine text extraction. Before upstream submission of Ethernet
support, validate the full GMII matrix once against the original schematic PDF.
