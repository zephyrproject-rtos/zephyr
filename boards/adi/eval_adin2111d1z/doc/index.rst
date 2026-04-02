.. zephyr:board:: adi_eval_adin2111d1z

Overview
********

Board EVAL-ADIN2111D1Z is a small 4.5x6.5 cm board with ADIN2111 and MAX32690
microcontroller.  The board provides two 10BASE-T1L ports and acts as an
evaluation node in a daisy-chain configuration for both power and data. A
sensor can be connected to the board, analog or digital. In case of power loss,
data is forwarded to another node/board in the chain. The purpose is not to
create a power node, but to demonstrate the daisy-chaining and bypassing data
an on-board relay.

Use-Cases
*********

- Daisy-chained sensors (temperature, pressure, light, proximity, ....) using
  10BASE-T1L; in a room / building
- Daisy-chained power over a number (?) of boards
- Daisy-chain data over a number (?) of boards
- Read data from on-board temperature sensor ADT75
- Demonstrate 10BASE-T1L communication over more nodes
- Use MAX32690 to control ADIN2111 over SPI interface

Hardware
********

+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Component                             | Function                                     | Description                                                                                                                                                                      |
+=======================================+==============================================+==================================================================================================================================================================================+
| ADIN2111                              | 2x 10BASE-T1L switch                         | Two port industrial ethernet switch with integrated 10BASE-T1L PHYs. Ultra-low power, various routing configurations. SPI communication to uC.                                   |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| MAX32690 (68 TQFN-EP)                 | ARM M4 Microcontroller (uC)                  | Widely used across ADI, 120 MHz clock speed, 3 MB flash, 1 MB SRAM, 12-bit ADC, Bluetooth LE 5.2; security options; bootloader. Interfaces: 2x I2C; I2S; SPI; 2x UART; OWM; USB. |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| MAX17640                              | DC/DC converter to 3.3V                      | Input +4.5V to +70V; output 400mA @3.3V. Temperature range -40 °C to +150 °C                                                                                                     |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| MAX77324                              | DC/DC converter to 1.8V                      | Input from MAX17640, output 1500mA @1.8V. VDDIO for ADIN2111 and uC, supply for other parts.                                                                                     |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| MAX77324                              | DC/DC converter to 1.1V                      | Input from MAX17640, output 1500mA @1.1V. Supplies the uC and ADIN2111.                                                                                                          |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| EEPROM Microchip AT24C64D             | EEPROM memory I2C to uC                      | 64-Kbit (8192 x 8), 400 KHz Fast mode                                                                                                                                            |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Polarity correction (Bourns CD-HD201) | Bridge rectifier                             | VRRM 100V, IFSM 50A; IF(AV) 2A; VF(MAX) 0.85A.                                                                                                                                   |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Relay (TE IM21GR)                     | Bypasses data when power lost                | VCOIL 3VDC, RCOIL 180 Ω; PCOIL 50 mW; Ultra highly sensitive. VSWITCH(MAX) 220VDC, ASWITCH(MAX) 2A.                                                                              |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Transformers (TDK ICI70CGI)           | Ensures high impedance in power loss         | LR 2.2 mH. Data and signal line chokes, without central tap.                                                                                                                     |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Power inductors (Coilcraft MSD7342)   | Power daisy-chain                            | L 47 μH; DCR(MAX) 0.42 Ω; SRF 9.5 MHz; ISAT(20% drop) 1.3 A; IRMS(both/one) = 0.61 / 0.86 A.                                                                                     |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| USB-C / JTAG                          | User / developer interfaces                  |                                                                                                                                                                                  |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| FTDI chip (FT230XQ-R)                 | Allows terminal connection to uC             | USB to serial UART interface.                                                                                                                                                    |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| ADT75 temp. sensor                    | Temperature sensor                           | Range −25 °C to +100 °C, ±2 °C. Power consumption 79 μW @3.3V                                                                                                                    |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| PMOD                                  |  UART/SPI/I2C connection for external sensor | 12-pin female connector.                                                                                                                                                         |
|                                       |  OWM (1-wire) and one ADC input              | Shares pins for ADC_0 and ADC_TRIG; OWM (1-wire)                                                                                                                                 |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| 6 pin header                          | SPI pins for ADIN2111                        | Only for testing                                                                                                                                                                 |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| BLE antenna (Kyocera M310220)         | 2.4 GHz on-board antenna                     | Freq. range 2400 –2485 MHz; G(PEAK) 1.7 dBi; linearly polarized.                                                                                                                 |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Port 1/2                              | 10BASE-T1L ports + DC power                  | 3-pin 2x Phoenix plug-in screw-terminal connectors                                                                                                                               |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Ext. power                            | External power (not injected to T1L lines)   | 2-pin Phoenix plug-in screw-terminal connector                                                                                                                                   |
+---------------------------------------+----------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

Supported Features
==================

.. zephyr:board-supported-hw::

Why MAX32690?
=============

Microcontroller ADI MAX32690 is an ARM Cortex-M4 device. It is ultra-efficient
and very low power, suitable for battery-powered devices. This microcontroller
(uC) is currently widely used across ADI. The uC has internal 3 MB flash
memory, which is necessary and enough for hosting a webpage and storing MAC and
IP addresses. Other uCs from the 326xx family have significantly less flash
memory (32670/1 has 384 KB; 32672 has 1 MB) and do not support external flash
memory. Other reasons for choosing MAX32690 are more GPIO pins, ability to
flash its firmware using USB interface and USB com port (not implemented yet at
the moment - planned), more security options; and also support for Bluetooth.

Also, this evaluation board has a representative value showing off our uC,
ADIN2111 PHY, ADT75 temp. sensor, MAX17640 and MAX77324 DC/DC converters; all
connected on one board.

Connectivity
============

- 2x 3 pin (+ - EARTH) Phoenix (P1, P2): Communication amplitude is 1V and 2.4V
  as well.
- USB-C connector (P4) uses FTDI chip to talk to the uC. A terminal app UART
  communication can be open to read/write to the uC. Also flashing a new
  firmware for the uC might be done using UCB-C port and bootloader.
- JTAG/SWD (P5) exposed single wire debug interface of the uC to the connector.
  A MAX32625PICO board can be used to download/debug the uC during firmware
  development.
- Bluetooth: A Bluetooth connectivity is provided by the uC MAX32690 and an
  integrated chip antenna on board.

Sensors
=======

+---------+-------+---------------+---------------------------------------------------------------------+
| Analog  | Type  | Connector     | Note                                                                |
+=========+=======+===============+=====================================================================+
| Digital | I2C   | PMOD (P6)     | All share one PMOD connector. Only one protocol can be used at once |
+---------+-------+---------------+                                                                     |
|         | SPI   | PMOD (P6)     |                                                                     |
+---------+-------+---------------+                                                                     |
|         | UART  | PMOD (P6)     |                                                                     |
+---------+-------+---------------+---------------------------------------------------------------------+
|         | ADT75 | on-board (U5) |                                                                     |
+---------+-------+---------------+---------------------------------------------------------------------+
|         | OWM   | PMOD (P6)     |                                                                     |
+---------+-------+---------------+---------------------------------------------------------------------+
| Analog  | ADC   | PMOD (P6)     | 12-bit SAR. PMOD only ADC trigger and ADC_0 input.                  |
+---------+       +---------------+---------------------------------------------------------------------+
|         |       | ADC_1 / ADC_4 | These pins available on the board (solder pins) for future use      |
+---------+-------+---------------+---------------------------------------------------------------------+


Power
=====

There are three options to provide power to the board: USB-C, Ext. 2-pin
connector, 10BASE-T1L 3-pin connectors.

The goal of the board is not to act as a power node, hence power provided from
different connectors will act differently.

- USB-C (P4)

    - Only for board - NOT injected to PORT1/2
    - Powers up DC/DC → all circuitry with 3V3, 1V8 and 1V1

- Ext. 2 pin connector (P3)

    - 5V-58VDC
    - Only for board - NOT injected to PORT1/2
    - Powers up DC/DC → all circuitry with 3V3, 1V8 and 1V1

- PORT 1

    - IN/OUT power
    - 7V-58VDC (voltage drop caused by bridge rectifier)
    - Power for the board
    - Powers up DC/DC → all circuitry with 3V3, 1V8 and 1V1
    - Forwarded to PORT 2 to daisy-chain power

- PORT 2

    - IN/OUT power
    - 7V-58VDC (voltage drop caused by bridge rectifier)
    - Power for the board
    - Powers up DC/DC → all circuitry with 3V3, 1V8 and 1V1
    - Forwarded to PORT 1 to daisy-chain power

Data/Power Bypass
=================

- Power to PORT 1

    - Board has power
    - Relay connects PORT 1 and PORT 2 to ADIN2111
    - Power is also forwarded to PORT 2

- Power to PORT 1 is lost

    - Board does not have power
    - Relay disconnects PORT 1 and PORT 2 from ADIN2111
    - Relay connects PORT 1 and PORT 2 together allowing daisy-chaining → data
      goes to another node in the chain
    - Power is NOT forwarded to PORT 2

- Data to PORT 1

    - Board does not have power UNLESS power provided from Ext. or USB-C
    - Relay connects PORT 1 and PORT 2 to ADIN2111 - Data from both ports goes
      to ADIN2111
    - Transformers ensure ADIN2111 is disconnected from the lines, resistance
      of lines connected to the transformers converges to infinite.

- Data to PORT 1 and power is lost

    - Relay connects PORT 1 and PORT 2 together to daisy-chain data
    - Relay connects PORT 1 and PORT 2 to ADIN2111
    - Power is NOT daisy-chained
    - Data is processed by ADIN2111
    - Board does not have power and power is NOT provided from Ext. or USB-C
    - Board does not have power, but power IS provided from Ext. or USB-C

Relay Functionality
===================

- Relay is controlled by uC GPIO pin (pin 13 / P0.23) connected to N-FET
  transistor

    - When configured correctly:
    - if uC has power → relay has power → PORT 1 and PORT 2 connected to
      ADIN2111
    - if uC hasn't got power → relay hasn't got power → PORT 1 and PORT 2 not
      connected to ADIN2111 instead connected together to allow daisy-chain data

- Inserting R41 will bypass uC controlling the relay => if board has power ->
  relay has power -> PORT 1 and PORT 2 connected to ADIN2111

    - This option is used in the first stage of testing HW and developing the
      board's SW.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::
