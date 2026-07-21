.. zephyr:board:: phyboard_atlas

Overview
********

The PHYTEC phyBOARD-Atlas i.MX RT1170 features the phyCORE-i.MX RT1170 System on
Module. The phyCORE-i.MX RT1170 module is based on the NXP dual core i.MX RT1170
which runs the Cortex-M7 core at 1 GHz and on the Cortex-M4 at 400 MHz.

Hardware
********

- MIMXRT1176DVMAA MCU

  - 1GHz Cortex-M7 & 400Mhz Cortex-M4
  - 2MB SRAM with 512KB of TCM for Cortex-M7 and 256KB of TCM for Cortex-M4

- Memory

  - 512 Mbit SDRAM
  - 128 Mbit QSPI Flash
  - 512 Mbit Octal Flash

- Interfaces

  - MicroSD: 1x microSD Card slot
  - USB: 2x USB 2.0 OTG
  - Ethernet: 1x 10/100/1000BASE-T & 1x 10/100BASE-T (RJ45)
  - Expansion: 1x M.2 Connector
  - Display: 1x MIPI-DSI
  - Camera: 1x MIPI CSI
  - Audio: Standard Audio Interface
  - Serial: 1x RS232 (2x5 pin header) + 1x RS485
  - UART: 1x via Expansion Interface
  - CAN: 1x CAN (2x5 pin header)
  - LED: 1x RGB User LED
  - Expansion Interface: 60-pin
  - Security: OPTIGA™ TPM SLB 9670 TPM 2.0
  - Power Input: USB-C 5V/3A

- Debug

   - Micro USB serial debug interface
   - JTAG: via Expansion Interface

For more information about phyCORE-i.MX RT1170 & phyBOARD-Atlas i.MX RT1170
board, see these references:

- `phyCORE-i.MX RT1170 Product Page`_
- `phyBOARD-Atlas i.MX RT1170 Product Page`_

External Memory
===============

This platform has the following external memories:

+----------------------+------------+-------------------------------------+
| Device               | Controller | Status                              |
+======================+============+=====================================+
| MT48LC16M16A2B4-7EIT | SEMC       | Enabled via device configuration    |
| SDRAM                |            | data (DCD) block, which sets up     |
|                      |            | the SEMC at boot time               |
+----------------------+------------+-------------------------------------+
| MX25U12832FM2I02     | FLEXSPI    | Enabled via flash configuration     |
| QSPI flash           |            | block (FCB), which sets up the      |
|                      |            | FLEXSPI at boot time.               |
+----------------------+------------+-------------------------------------+

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and I/Os
====================

The MIMXRT1170 SoC has six pairs of pinmux/gpio controllers.

+---------------------------+----------------------+-------------------------------+
| Name                      | Function             | Usage                         |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_35                | GPIO                 | User button                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_SNVS_08              | GPIO                 | Red LED (SOM)                 |
+---------------------------+----------------------+-------------------------------+
| GPIO_SNVS_09              | GPIO                 | Green LED (SOM)               |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_14                | GPIO                 | Red LED (Carrier Board)       |
+---------------------------+----------------------+-------------------------------+
| GPIO_LPSR_13              | GPIO                 | Green LED (Carrier Board)     |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_18                | LPI2C2_SCL           | EEPROM                        |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_19                | LPI2C2_SDA           | EEPROM                        |
+---------------------------+----------------------+-------------------------------+
| GPIO_LPSR_08              | LPI2C5_SCL           | Accelerometer, CODEC, Display |
+---------------------------+----------------------+-------------------------------+
| GPIO_LPSR_09              | LPI2C5_SDA           | Accelerometer, CODEC, Display |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_24                | LPUART1_TX           | UART Console (CM7)            |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_25                | LPUART1_RX           | UART Console (CM7)            |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_28                | LPUART5_TX           | UART                          |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_29                | LPUART5_RX           | UART                          |
+---------------------------+----------------------+-------------------------------+
| GPIO_EMC_B1_40            | LPUART6_TX           | UART Console (CM4)            |
+---------------------------+----------------------+-------------------------------+
| GPIO_EMC_B1_41            | LPUART6_RX           | UART Console (CM4)            |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_00                | CTP_INT              | Display                       |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_01                | CTP_RST_B            | Display                       |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B2_05           | PWR_EN               | Display                       |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_30                | Backlight_CTL        | Display                       |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_32                | ENET_MDC             | Ethernet                      |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_33                | ENET_MDIO            | Ethernet                      |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B2_02           | ENET_TX_DATA00       | Ethernet                      |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B2_03           | ENET_TX_DATA01       | Ethernet                      |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B2_04           | ENET_TX_EN           | Ethernet                      |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B2_05           | ENET_TX_CLK          | Ethernet                      |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B2_06           | ENET_RX_DATA00       | Ethernet                      |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B2_07           | ENET_RX_DATA01       | Ethernet                      |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B2_08           | ENET_RX_EN           | Ethernet                      |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B2_09           | ENET_RX_ER           | Ethernet                      |
+---------------------------+----------------------+-------------------------------+
| GPIO_EMC_B2_19            | ENET_RGMII_MDC       | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_EMC_B2_20            | ENET_RGMII_MDIO      | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_09           | ENET_RGMII_TX_DATA00 | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_08           | ENET_RGMII_TX_DATA01 | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_07           | ENET_RGMII_TX_DATA02 | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_06           | ENET_RGMII_TX_DATA03 | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_10           | ENET_RGMII_TX_EN     | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_11           | ENET_RGMII_TX_CLK    | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_02           | ENET_RGMII_RX_DATA00 | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_03           | ENET_RGMII_RX_DATA01 | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_04           | ENET_RGMII_RX_DATA02 | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_05           | ENET_RGMII_RX_DATA03 | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_00           | ENET_RGMII_RX_EN     | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_DISP_B1_01           | ENET_RGMII_RX_CLK    | Ethernet 1G                   |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_02                | LPUART8_TX           | RS-232                        |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_03                | LPUART8_RX           | RS-232                        |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_04                | LPUART8_CTS          | RS-232                        |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_05                | LPUART8_RTS          | RS-232                        |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B2_05             | FLEXSPI1_DQS         | Flash Programming             |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B2_06             | FLEXSPI1_SS0         | Flash Programming             |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B2_07             | FLEXSPI1_SCLK        | Flash Programming             |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B2_08             | FLEXSPI1_DATA00      | Flash Programming             |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B2_09             | FLEXSPI1_DATA01      | Flash Programming             |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B2_10             | FLEXSPI1_DATA02      | Flash Programming             |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B2_11             | FLEXSPI1_DATA03      | Flash Programming             |
+---------------------------+----------------------+-------------------------------+
| GPIO_LPSR_00              | CAN3_TX              | FlexCAN                       |
+---------------------------+----------------------+-------------------------------+
| GPIO_LPSR_01              | CAN3_RX              | FlexCAN                       |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_17_SAI1_MCLK      | SAI_MCLK             | CODEC/SAI                     |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_20_SAI1_RX_DATA00 | SAI1_RX_DATA00       | CODEC/SAI                     |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_21_SAI1_TX_DATA00 | SAI1_TX_DATA00       | CODEC/SAI                     |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_22_SAI1_TX_BCLK   | SAI1_TX_BCLK         | CODEC/SAI                     |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_23_SAI1_TX_SYNC   | SAI1_TX_SYNC         | CODEC/SAI                     |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B1_00             | USDHC1_CMD           | SDHC                          |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B1_01             | USDHC1_CLK           | SDHC                          |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B1_02             | USDHC1_DATA0         | SDHC                          |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B1_03             | USDHC1_DATA1         | SDHC                          |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B1_04             | USDHC1_DATA2         | SDHC                          |
+---------------------------+----------------------+-------------------------------+
| GPIO_SD_B1_05             | USDHC1_DATA3         | SDHC                          |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_09                | USB_OTG1_ID          | USB                           |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_10                | USB_OTG1_PWR         | USB                           |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_11                | USB_OTG1_OC          | USB                           |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_08                | USB_OTG2_ID          | USB                           |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_07                | USB_OTG2_PWR         | USB                           |
+---------------------------+----------------------+-------------------------------+
| GPIO_AD_06                | USB_OTG2_OC          | USB                           |
+---------------------------+----------------------+-------------------------------+

Dual Core samples
*****************

+-----------+------------------+------------------+
| Core      | Boot Address     | Comment          |
+===========+==================+==================+
| Cortex M7 | 0x30000000[630K] | primary core     |
+-----------+------------------+------------------+
| Cortex M4 | 0x20020000[96k]  | boots from OCRAM |
+-----------+------------------+------------------+

+----------+------------------+-----------------------+
| Memory   | Address[Size]    | Comment               |
+==========+==================+=======================+
| flexspi1 | 0x30000000[16M]  | Cortex M7 flash       |
+----------+------------------+-----------------------+
| sdram0   | 0x80030000[64M]  | Cortex M7 ram         |
+----------+------------------+-----------------------+
| ocram    | 0x20020000[512K] | Cortex M4 "flash"     |
+----------+------------------+-----------------------+
| sram1    | 0x20000000[128K] | Cortex M4 ram         |
+----------+------------------+-----------------------+
| ocram2   | 0x200C0000[512K] | Mailbox/shared memory |
+----------+------------------+-----------------------+

Only the first 16K of ocram2 has the correct MPU region attributes set to be
used as shared memory

System Clock
============

The MIMXRT1170 SoC is configured to use SysTick as the system clock source,
running at 996MHz. When targeting the M4 core, SysTick will also be used,
running at 400MHz

When power management is enabled, the 32 KHz low frequency
oscillator on the board will be used as a source for the GPT timer to
generate a system clock. This clock enables lower power states, at the
cost of reduced resolution

Serial Port
===========

The MIMXRT1170 SoC has 12 UARTs. ``LPUART1`` is configured for the console for
the CM7 and ``LPUART6`` is configured for the console of the CM4. ``LPUART8`` is
configured for RS-232. Remaining are not used.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Console
=====================

We will use the on-board FTDI UART-to-USB IC that converts the two internal
UARTs to USB (X15).

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: phyboard_atlas/mimxrt1176/cm7
   :goals: flash

You should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.1.0-xxxx-xxxxxxxxxxxxx *****
   Hello World! phyboard_atlas/mimxrt1176/cm7

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: phyboard_atlas/mimxrt1176/cm7
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.1.0-xxxx-xxxxxxxxxxxxx *****
   Hello World! phyboard_atlas/mimxrt1176/cm7

.. note::
   Use J-Link version 7.50 or later. Debugging only supports running the CM4 image, since the board’s default boot core is CM7.

References
**********

.. target-notes::

.. _phyCORE-i.MX RT1170 Product Page:
   https://www.phytec.com/product/phycore-rt1170

.. _phyBOARD-Atlas i.MX RT1170 Product Page:
   https://www.phytec.com/product/phyboard-rt1170-development-kit/

.. _AN13264:
   https://www.nxp.com/docs/en/application-note/AN13264.pdf
