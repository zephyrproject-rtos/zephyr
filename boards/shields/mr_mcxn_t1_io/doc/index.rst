.. _mr_mcxn_t1_io_shield:

MR-MCXN-T1-IO Flex IO Shield
############################

.. figure:: mr_mcxn_t1_io.webp
   :align: center
   :alt: MR-MCXN-T1-IO Flex IO Shield

   MR-MCXN-T1-IO Flex IO Shield

Overview
********

The MR-MCXN-T1-IO Flex IO expansion board is a passive breakout stacking shield for
the :zephyr:board:`mr_mcxn_t1`. It carries no fixed silicon; instead it routes
a set of the board's peripheral buses to expansion connectors:

- One expansion I2C bus (FlexCOMM3)
- Two UARTs with hardware flow control (FlexCOMM0 and FlexCOMM2)
- Two FlexPWM blocks, four channels each (FlexPWM0 and FlexPWM1)
- One ADC (LPADC1)
- Two SPI buses on JST-GH connectors (FlexCOMM6 and FlexCOMM7)

The shield overlay enables these buses. Because the board is passive, the
devices attached to the expansion connectors, the ADC channel configuration
and any PWM consumers are defined by the application overlay.

Requirements
************

This shield mates the MR-MCXN-T1 stacking header and works only with
the :zephyr:board:`mr_mcxn_t1` board. Its FlexPWM channels and the expansion
I2C share port-3 and FlexCOMM3 pins with the MR-MCXN-T1-F9P GNSS and
MR-MCXN-T1-OFL optical-flow shields, so it cannot be stacked together with
either of them.

Pin Assignments
***************

+-----------------------+-----------------------------------------------+
| Function              | MR-MCXN-T1 resource                           |
+=======================+===============================================+
| Expansion I2C         | FlexCOMM3 (LPI2C)                             |
+-----------------------+-----------------------------------------------+
| UART 0 (flow control) | FlexCOMM0 (LPUART)                            |
+-----------------------+-----------------------------------------------+
| UART 2 (flow control) | FlexCOMM2 (LPUART)                            |
+-----------------------+-----------------------------------------------+
| PWM 0                 | FlexPWM0 (4 channels)                         |
+-----------------------+-----------------------------------------------+
| PWM 1                 | FlexPWM1 (4 channels)                         |
+-----------------------+-----------------------------------------------+
| ADC                   | LPADC1                                        |
+-----------------------+-----------------------------------------------+
| SPI 1 and SPI 2       | FlexCOMM6 and FlexCOMM7 (LPSPI)               |
+-----------------------+-----------------------------------------------+

Connectors
**********

Each ESC connector provides two LPADC1 inputs. The VBAT pin is sensed on
channels A18 (ESC1) and A16 (ESC2) through a 33k and 2k2 divider, so the ADC
reads one sixteenth of the pack voltage and supports packs up to 12S. The
CURR pin is sensed on channels A23 (ESC1) and A17 (ESC2) through a 10 ohm
series filter with no divider fitted, reading the analog current telemetry
one to one up to 3.3 V, and an unpopulated shunt footprint on each CURR
channel allows custom scaling. The TELEM pin on both ESC connectors joins a
shared LP_FLEXCOMM9 UART line on P2_3.

The QDEC connector provides two quadrature encoder inputs with a filtered
5 V supply. QDC1 phase A and B arrive on P4_20 and P1_22 and QDC2 phase A
and B on P2_6 and P2_0, which reach the SoC quadrature decoders through
input multiplexer triggers. The shield leaves the qdc0 and qdc1 nodes
disabled because counts-per-revolution is a property of the attached
encoder, so applications enable the decoder with their encoder settings or
use the pins as general purpose inputs.

The GPS connector follows the Pixhawk 10 pin GPS pinout. It carries 5 V, the
LP_FLEXCOMM3 UART on P1_2 and P1_3, the LP_FLEXCOMM3 I2C bus on P1_0 and
P1_1 shared with the I2C2 connector for a compass, the safety switch input
on P2_9 and the safety LED output on P2_8 with a 3.3 V supply for the switch
LED, a buzzer output driven by an on shield low side transistor from CTIMER1
with a physical disable switch, and ground.

The SPI1 and SPI2 connectors expose the shield header's FlexCOMM6 and
FlexCOMM7 SPI buses with 5 V power and an ESD protected host request line
per connector, all signals at 3.3 V logic. SPI1 uses chip select SSEL2 and
SPI2 uses SSEL0, the same chip selects the flow shield position assigns to
its IMUs, so application overlays define the attached devices and their
selects.

The RTC_BAT connector accepts a backup battery that feeds the board's
MCXN947 VDD_BAT rail so the RTC keeps running while the system is
unpowered, and a diode with a 75 ohm series path from 3.3 V maintains the
rail while main power is present.

Programming
***********

Set ``--shield mr_mcxn_t1_io`` when building. The application overlay
adds the devices on the expansion connectors. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mr_mcxn_t1/mcxn947/cpu0
   :shield: mr_mcxn_t1_io
   :goals: build flash
   :compact:

References
**********

- `MR-MCXN-T1 shield hardware design files (KiCad) <https://github.com/CogniPilot/spinali_mcxn_t1_shields>`_
