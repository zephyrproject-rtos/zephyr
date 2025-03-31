.. zephyr:board:: max32675evkit

Overview
********
The MAX32675 evaluation kit (EV kit) provides a platform for evaluation capabilities of
the MAX32675 microcontroller, which is a highly integrated, mixed-signal, ultralow-power
microcontroller designed for industrial and medical sensors. It contains an integrated, low-power
HART modem which enables the bidirectional transfer of digital data over a current loop, to/from
industrial sensors for configuration and diagnostics.

The Zephyr port is running on the MAX32675 MCU.

Hardware
********

- MAX32675 MCU:

  - Low-Power, High-Performance for IndustrialApplications

    - 100MHz Arm Cortex-M4 with FPU
    - 384KB Internal Flash
    - 160KB SRAM
    - 128kB ECC Enabled
    - 44.1μA/MHz ACTIVE Mode at 0.9V up to 12MHzCoremark®
    - 64.5μA/MHz ACTIVE Mode at 1.1V up to 100MHzCoremark
    - 2.84μA Full Memory Retention Current in BACKUPMode at VDDIO = 3.3V
    - Ultra-Low-Power Analog Peripherals

  - Optimal Peripheral Mix Provides Platform Scalability

    - Two Sigma-Delta ADCs
    - 12 Channels, Assignable to Either ADC
    - Flexible Resolution and Sample Rates (24 Bits at 0.4ksps, 16 Bits at 4ksps)
    - 12-Bit DAC
    - On-Die Temperature Sensor
    - SPI (M/S)
    - Up to Two I2C
    - Up to Two UARTs
    - Up to 23 GPIOs
    - Up to Five 32-Bit Timers
    - Two Windowed Watchdog Timers
    - 8-Channel Standard DMA Controller
    - One I2S Slave for Digital Audio Interface

  - Robust Security and Reliability

    - TRNG Compliant to SP800-90B
    - Secure Nonvolatile Key Storage and AES-128/192/256
    - Secure Bootloader to Protect IP/Firmware
    - Wide, -40°C to +105°C Operating TemperatureRange


- Benefits and Features of MAX32675EVKIT:

    - HART Compatible Secondary Master with the Ability to Connect to Existing 4-20mA Current Loop and Communicate with HART Enabled Devices
    - USB 2.0 Micro B to Serial UART
    - Two On-Board, High-Precision Voltage References
    - All GPIOs Signals Accessed Through 0.1in Headers
    - Access to 4 Analog Inputs Through SMA Connectors Configured as Differential
    - Access to 8 Analog Inputs Through 0.1in Headers Configured as Single-Ended
    - DAC Output Accessed Through SMA Connector or Test Point
    - 10-Pin SWD and Connector
    - Board Power Provided by USB Port
    - On-Board 1.0V, 1.8V, and 3.3V LDO Regulators
    - Individual Power Measurement on all IC Rails Through Jumpers
    - Two General-Purpose LEDs and Two General-Purpose Pushbutton Switches

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| Name      | Name          | Settings      | Description                                                                                      |
+===========+===============+===============+==================================================================================================+
| JP1       | P1_9          |               |                                                                                                  |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects red LED D1 from P1_9.                                             |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects red LED D1 to P1_9.                                                  |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP2       | P1_10         | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects green LED D2 from P1_10.                                          |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects green LED D2 to P1_10.                                               |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP3       | I2C_SCLK      | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects 3V3 from I2C_SCLK.                                                |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects 3V3 to I2C0_SCLK.                                                    |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP4       | I2C_SDA       | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects 3V3 to I2C_SDA.                                                   |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects 3V3 to I2C_SDA.                                                      |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP5       | UART0_RX      | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects UART0_RX (P0.8) from the SWD connector.                           |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects UART0_RX (P0.8) to the SWD connector.                                |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP6       | UART0_TX      | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disonnects UART0_TX (P0.9) from the SWD connector.                            |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects UART0_TX (P0.9) to the SWD connector.                                |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP7       | REF0N         | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects REF0N from ground.                                                |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects REF0N to ground.                                                     |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP8       | REF1N         | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects REF1N from ground.                                                |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects REF1N to ground.                                                     |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP9       | +-----------+ | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           | | HART_IN   | | | Open      | |  | Disconnects TX of USB - serial bridge from HART_IN (P0.15).                   |               |
|           | +-----------+ | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           | | HART_IN   | | | 1-2       | |  | Connects TX of USB - serial bridge to HART_IN (P0.15).                        |               |
|           | +-----------+ | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           | | HART_OUT  | | | Open      | |  | Disconnects RX of USB - serial bridge from HART_OUT (P0.14).                  |               |
|           | +-----------+ | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           | | HART_OUT  | | | 3-4       | |  | Connects RX of USB - serial bridge to HART_OUT (P0.14).                       |               |
|           | +-----------+ | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           | | HART_RTS  | | | Open      | |  | Disconnects RTS of USB - serial bridge from HART_RTS (P1.8).                  |               |
|           | +-----------+ | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           | | HART_RTS  | | | 4-5       | |  | Connects TX of USB - serial bridge to HART_RTS (P1.8).                        |               |
|           | +-----------+ | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           | | HART_OCD  | | | Open      | |  | Disconnects RTS of USB - serial bridge from HART_OCD (P0.16).                 |               |
|           | +-----------+ | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           | | HART_OCD  | | | 7-8       | |  | Connects TX of USB - serial bridge to HART_OCD (P0.16).                       |               |
|           | +-----------+ | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP10      | SWD_CLK       | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects boot load enable circuit from SWD_CLK (P0.1).                     |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects boot load enable circuit to SWD_CLK (P0.1).                          |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP11      | FSK_IN        | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects FSK_IN from HART analog circuitry.                                |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects FSK_IN to HART analog circuitry.                                     |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP12      | FSK_OUT       | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects FSK_OUT from HART analog circuitry.                               |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects FSK_OUT to HART analog circuitry.                                    |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP13      | RCV_FSK       | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects RCV_FSK from CC LOOP.                                             |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects RCV_FSK to CC LOOP.                                                  |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP14      | RCV_FSK       | +-----------+ |  +--------------------------------------------------------------------------------+              |
|           |               | | Open      | |  | Disconnects RCV_FSK from XFMR LOOP.                                            |              |
|           |               | +-----------+ |  +--------------------------------------------------------------------------------+              |
|           |               | | Closed    | |  | Connects RCV_FSK to XFMR LOOP.                                                 |              |
|           |               | +-----------+ |  +--------------------------------------------------------------------------------+              |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP15      | RLOAD         | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects 249Ω resistor shunt from CC LOOP.                                 |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects 249Ω resistor shunt to CC LOOP.                                      |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP16      | N/A           | N/A           |  N/A                                                                                             |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP17      | N/A           | N/A           |  N/A                                                                                             |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP18      | N/A           | N/A           |  N/A                                                                                             |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP19      | HART_RTS      | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Enables HART_RTS optical transceiver.                                         |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Bypasses HART_RTS optical transceiver.                                        |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP20      | RLOAD         | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects 249Ω resistor shunt from XFMR LOOP.                               |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects 249Ω resistor shunt to XFMR LOOP.                                    |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP21      | VDDIO         | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects power from VDDIO.                                                 |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects power to VDDIO.                                                      |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP22      | VDDA          | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects power from VDDA.                                                  |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects power to VDDA.                                                       |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP23      | VDD18         | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects power from VDD18.                                                 |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects power to VDD18.                                                      |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP24      | VCORE         | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Open      | |  | Disconnects power from VCORE.                                                 |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | Closed    | |  | Connects power to VCORE.                                                      |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP25      | REF0P         | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 2-1       | |  | Connects OB_VREF to REF0P.                                                    |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 2-3       | |  | Connects INT_VREF to REF0P.                                                   |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+
| JP26      | REF1P         | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 2-1       | |  | Connects OB_VREF to REF1P.                                                    |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               | | 2-3       | |  | Connects INT_VREF to REF1P.                                                   |               |
|           |               | +-----------+ |  +-------------------------------------------------------------------------------+               |
|           |               |               |                                                                                                  |
+-----------+---------------+---------------+--------------------------------------------------------------------------------------------------+


Detailed Description of Hardware
================================

HART Interface
**************
The HART circuitry acts as a secondary master with the ability to connect to an existing 4mA–20mA
current loop and communicates with HART-enabled devices. Connection to a capacitance coupled loop
through JH8 and a transformer loop is through JH9. HART communication to the MAX32675 is through
the USB connector CN1.

USB-to-HART Interface
*********************
The EV kit provides a USB-to-HART bridge chip, FTDI FT231. This bridge eliminates the requirement
for a physical RS-232 COM port. Instead, the IC’s HART access is through the Micro-USB type-B
connector, CN1. Virtual COM port drivers and guides for installing Windows® drivers are available
at the FTDI chip website.

Power Supply
************
The EV kit is powered by +5V that is made available through VBUS on the Micro-USB type-B
connector CN1. A blue LED (D5) illuminates when the board is powered. Green LEDs (D6), (D7),
and (D8) illuminate when the 3V3, 1V8, and 1V0 LDOs are powered, respectively.

Current Monitoring
******************
Two pin headers provide convenient current monitoring points for VDDIO EN (JP21),
VDDA EN (JP22), VDD18 EN (JP23), and VCORE (JP24).
To accurately achieve the low-power current values, the EVkit needs to be configured
such that no outside influence (i.e., pullups, external clock, debugger connector, etc.)
causes a current source or sink on that GPIO.

Clocking
********
The MAX32675 clocking is provided by an external 16MHz crystal (Y1).

Voltage Reference
*****************
The differential reference inputs REF0 and REF1 can be sourced by an internal reference (INT_VREF)
or a higher precision external reference source, MAX6071.
This is selected by jumpers JP25 and JP26.

UART Interface
**************
The EV kit provides a USB-to-UART bridge chip (the FTDI FT230XS-R). This bridge eliminates
the requirement for a physical RS-232 COM port. Instead, the IC’s UART access is through
the Micro USB type-B connector (CN1). The USB-to-UART bridge can be connected to the IC’s UART0
or LPUART0 with jumpers JP10 (RX0) and JP11 (TX0). Virtual COM port drivers and guides for
installing Windows® drivers are available on the FTDI Chip website.

Boot Loader
***********
Boot load is activated by boot load enable slide switch SW5.

GPIO and Alternate Function Headers
***********************************
GPIO and alternate function signals from the MAX32675 can be accessed through 0.1in
spaced headers JH1, JH2, JH3, and JH4.

Analog Input Access
*******************
Analog inputs (AIN0–AIN3) can be accessed differentially from SMA connectors J2 and J3 or
separately from TP10, TP12, TP15, and TP16, respectively. Analog inputs (AIN4–AIN11) can be
accessed through 0.1in spaced headers JH5 and JH6.

I2C Pullups
***********
The I2C port can independently pulled up to 3V3 through JP3 (I2C_SCL) and JP4 (I2C_SDA).

Reset Pushbutton
****************
The IC can be reset by pushbutton SW3.

Indicator LEDs
**************
General-purpose indicators LED D1 (red) is connected to GPIO P1.9 and LED D2 (green) is connected
to GPIO P1.10.

GPIO Pushbutton Switches
************************
The two general-purpose pushbuttons (SW1 and SW2) are connected to GPIO P1.11 and P1.12,
respectively. If the pushbutton is pressed, the attached port pin is pulled low.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

SWD debug can be accessed through an Arm Cortex 10-pin connector (J5).
Logic levels are set to 3V3 by default, but they can be set to 1.8V if TP5 (VDD_VDDA_EXT)
is supplied externally. Be sure to remove jumper JP15 (LDO_DUT_EN) to disconnect
the 3.3V LDO if supplying VDD and VDDA externally.

Once the debug probe is connected to your host computer, then you can simply run the
``west flash`` command to write a firmware image into flash. To perform a full erase,
pass the ``--erase`` option when executing ``west flash``.

.. note::

   This board uses OpenOCD as the default debug interface. You can also use
   a Segger J-Link with Segger's native tooling by overriding the runner,
   appending ``--runner jlink`` to your ``west`` command(s). The J-Link should
   be connected to the standard 2*5 pin debug connector (JH2) using an
   appropriate adapter board and cable.

Debugging
=========

Please refer to the `Flashing`_ section and run the ``west debug`` command
instead of ``west flash``.

References
**********

- `MAX32675EVKIT web page`_

.. _MAX32675EVKIT web page:
   https://www.analog.com/en/design-center/evaluation-hardware-and-software/evaluation-boards-kits/max32675evkit.html
