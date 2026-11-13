.. zephyr:board:: hpm5300evk

Overview
********

The HPM5300EVK is a development and evaluation board for the HPMicro HPM5300
series of RISC-V microcontrollers. This board configuration targets the
HPM5361ICB1 device in the LQFP-100 package.

The HPM5361ICB1 is a 32-bit RISC-V microcontroller clocked at up to 480 MHz.
The core implements the RV32-IMAFDCBP instruction set, with 16 KiB L1
instruction and data caches, 128 KiB instruction local memory (ILM), 128 KiB
data local memory (DLM), 32 KiB AHB SRAM (288 KiB total), 1 MiB on-chip flash,
128 KiB boot ROM, and 4096-bit OTP.

The SoC provides a motion-control subsystem with two 8-channel PWM timers, two
quadrature encoder inputs (QEIv2), two quadrature encoder outputs (QEO), two
serial encoder interfaces (SEI), two motion management controllers (MMC), a
resolver decoder (RDC), a programmable logic block (PLB), and a trigger manager.
Communication peripherals include nine UARTs, four SPI buses, four I2C buses,
four CAN-FD controllers, and one USB 2.0 OTG controller with an integrated
high-speed PHY. Analog peripherals include two 16-bit ADCs, two 12-bit DACs,
two analog comparators, and two operational amplifiers.

The board exposes these capabilities through a 32-pin motor-control connector,
a 2x20-pin Raspberry Pi-compatible expansion header, a CAN/LIN connector, and
RS-485/RS-422 transceivers. It also provides a 1 MiB QSPI NOR flash, a 24 MHz
crystal, two USB Type-C ports, an on-board FT2232HL debug probe with JTAG and
UART bridges, a 20-pin JTAG connector, an ADC input SMA, user and system keys,
a boot-mode switch, and a user LED.

See Supported Features for the peripherals supported by Zephyr on this board.

Hardware
********

- HPMicro HPM5361ICB1 RISC-V microcontroller, up to 480 MHz, 288 KiB SRAM,
  1 MiB on-chip flash, and a 24 MHz crystal
- 1 MiB QSPI NOR flash for XIP
- 2 USB Type-C ports: one USB 2.0 OTG port and one debug port connected to an
  on-board FT2232HL debug probe
- 1 20-pin JTAG connector
- 1 2x20-pin Raspberry Pi-compatible expansion header
- 1 32-pin motor-control connector with three-phase PWM and QEO signals
- 1 5-pin CAN/LIN connector with one CAN-FD channel and one LIN channel
- 1 RS-485 transceiver and one RS-422 transceiver connected to SEI1
- 1 3-pin SEI clock-direction selection header
- 1 ADC input SMA
- Reset, wakeup, and user keys; 2-position boot-mode DIP switch; user LED

.. note::
   On the HPM5300EVK RevC and earlier versions of the development board,
   R37 is fitted with 1 kOhm instead of the required 10 Ohm. This may prevent
   the USB device from being enumerated. RevD uses the correct 10 Ohm
   resistor. See the `HPM5300EVK board known issue`_ for details.

Connections and IOs
===================

The following table lists the main board-level interfaces. The 2x20 expansion
header also exposes additional GPIOs; see the `HPM5300EVK board documentation`_
for the complete pinout.

.. list-table::
   :header-rows: 1

   * - Interface
     - SoC pins
     - Board connection
     - Description
   * - UART0 console
     - PA00 (TX), PA01 (RX)
     - J8 (USB DEBUG Type-C) through the FT2232HL
     - 115200 8N1 console
   * - JTAG
     - PA04 (TDO), PA05 (TDI), PA06 (TCK), PA07 (TMS), PA08 (TRST)
     - J2 and FT2232HL
     - Debug and programming
   * - USB0 D+/D-
     - USB_DP, USB_DM
     - J1 (USB0_P/USB0_N)
     - USB 2.0 high-speed OTG
   * - Motor PWM
     - PA26-PA29 (PWM0 P2-P5)
     - J7[11], J7[12], J7[9], J7[10]
     - Three-phase motor-control PWM
   * - QEO0 A/B/Z
     - PA20 (A), PA21 (B), PA22 (Z)
     - J7[26], J7[24], J7[22]
     - Quadrature encoder output
   * - UART2
     - PB08 (TX), PB09 (RX), PB10 (DE)
     - P1[8], P1[10], P1[38]
     - Expansion-header UART
   * - SPI1
     - PA26 (CS0), PA27 (SCLK), PA28 (MISO), PA29 (MOSI)
     - P1[24], P1[23], P1[21], P1[19]
     - Expansion-header SPI
   * - I2C0
     - PB02 (SCL), PB03 (SDA)
     - P1[28], P1[27]
     - Expansion-header I2C
   * - ADC0
     - PB05 (ADC0.INA13)
     - RF1 (ADC SMA) and P1[32]
     - 16-bit ADC input
   * - User LED
     - PA23
     - LED2
     - Active low
   * - User key
     - PA09
     - SW5
     - Active low

On J2, the JTAG signals are on pins 3 (TRST), 5 (TDI), 7 (TMS), 9 (TCK),
and 13 (TDO).

.. note::

   Several SoC pins are routed to more than one connector. PA26-PA29 are
   shared between SPI1 and PWM0, and PB08/PB09 are shared between UART2 and
   DAC0/DAC1. Only one peripheral function can use a shared pin at a time.

The following on-board transceivers connect the SoC communication peripherals
to the external connectors:

.. list-table::
   :header-rows: 1

   * - Transceiver
     - SoC peripheral
     - SoC pins
     - External connection
   * - TJA1042T/3 CAN-FD
     - MCAN3
     - PY05 (TX), PY04 (RX)
     - J9[3] (CAN_H), J9[1] (CAN_L)
   * - TJA1021T LIN
     - UART3 in LIN mode
     - PA14 (RX), PA15 (TX), PA13 (SLP_N), PA24 (12 V enable)
     - LIN on J9[5]
   * - MAX3485EESA+T
     - SEI1 data (RS-485 and RS-422)
     - PA16 (DE), PA18 (TX), PA19 (RX)
     - J7[21] (DATA_N), J7[23] (DATA_P)
   * - SIT3490EESA
     - SEI1 clock (RS-422)
     - PA17 (CK)
     - J7[25]/J7[27]/J7[29]/J7[31]; J5 selects the clock direction

See the `HPM5300EVK board documentation`_ and `HPM5300EVK RevC schematic`_
for the full board pinout.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The on-board FT2232HL provides both JTAG and the UART bridge. Five jumpers
(JP3-JP7) connect it to the MCU JTAG signals:

- To use the external JTAG connector (J2), remove all five jumpers.
- To use the on-board debugger through the USB DEBUG port, install all five
  jumpers.

Applications can be built and flashed as usual:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: hpm5300evk/hpm5361
   :goals: build flash

For a RAM build, disable XIP:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: hpm5300evk/hpm5361
   :goals: build flash
   :gen-args: -DCONFIG_XIP=n

Boot mode
=========

The HPM5300EVK uses a 2-position DIP switch to select the boot mode.
DIP switch position 1 controls BOOT1 (PA03), and position 2 controls BOOT0
(PA02). ON is logic 1 and OFF is logic 0.

.. list-table::
   :header-rows: 1

   * - BOOT1 (PA03)
     - BOOT0 (PA02)
     - Boot mode
   * - OFF
     - OFF
     - QSPI NOR boot
   * - OFF
     - ON
     - Serial boot
   * - ON
     - OFF
     - ISP
   * - ON
     - ON
     - Retention mode

Setting both switches to OFF selects the built-in QSPI NOR flash used by the
default XIP build.

References
**********

- `HPM5300EVK product page`_
- `HPM5300 datasheet`_
- `HPM5300EVK board documentation`_
- `HPM5300EVK user manual`_
- `HPM5300EVK RevC schematic`_

.. _HPM5300EVK product page:
   https://www.hpmicro.com/product-center/microcontroller/hpm5300

.. _HPM5300 datasheet:
   https://www.hpmicro.com/Public/Uploads/uploadfile/files/20251126/HPM5300DSV012.pdf

.. _HPM5300EVK board documentation:
   https://hpm-sdk.readthedocs.io/en/v1.13.0/boards/hpm5300evk/README_en.html

.. _HPM5300EVK board known issue:
   https://hpm-sdk.readthedocs.io/en/v1.13.0/boards/hpm5300evk/README_en.html#board-known-issue

.. _HPM5300EVK user manual:
   https://www.hpmicro.com/Public/Uploads/uploadfile/files/20241024/HPM5300EVKUGV1.0.pdf

.. _HPM5300EVK RevC schematic:
   https://www.hpmicro.com/Public/Uploads/uploadfile/files/20240620/HPM5300EVKREVC.pdf
