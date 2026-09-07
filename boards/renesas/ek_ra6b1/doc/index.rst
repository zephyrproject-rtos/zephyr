.. zephyr:board:: ek_ra6b1

Overview
********

The EK-RA6B1 DEVKIT-PRO is a development kit for the Renesas RA6B1 Group, which
features a dual-core multi-protocol wireless microcontroller.
The SoC combines an Arm Cortex-M33 application processor with an Arm Cortex-M0+
software-configurable protocol engine for wireless connectivity.

**MCU Native Pin Access**

- RA6B1 Group MCU (R7KA6B1-xxxxxx) on the daughterboard module (744-13-x) mounted on the
  mainboard (744-04-x)
- Application Processor: Arm Cortex-M33 up to 128 MHz with FPU, DSP, and TrustZone
- Protocol Processor: Arm Cortex-M0+ for Bluetooth LE and 802.15.4 MAC
- Memory: Up to 1536 KB MRAM, 512 KB SRAM, 128 KB ROM
- 47 GPIOs: P0xx (32x 1.8V), P1xx (6x 3.3V + USB), P2xx (7x 1.8V)
- Multiple clock sources: 32 MHz XTAL, 32.768 kHz XTAL, internal RC oscillators

**System Control and Ecosystem Access**

- Power inputs: VBAT (1.62-5.75V), VUSB, VDDIO (1.8-3.3V)
- Integrated SIMO DC-DC converter with external inductor connections (LX, LY)
- SEGGER J-Link onboard debugger
- PMM2 Power Measurement Module for current consumption analysis
- Reset: nRST pin + GPIO power-on reset capability

**User LEDs and Buttons**

- User LED: LED0
- Power LED indicating regulated power availability
- Debug LED indicating J-Link connection status
- Reset button (nRST)
- User buttons via GPIO pins

**Debug Interfaces**

- ETM Trace: P019 (TRACECLK), P020-P023 (TRACEDATA0-3)
- UART Boot: P012 (TX), P013 (RX)
- J-Link USB connection with RA6B1 support package

**Special Feature Access**

- Radio

  - Transmitter output power: -20dBm to 10dBm
  - Receiver Sensitivity: -100dBm at 1Mbps BLE and -105dBm at 250ksps 802.15.4
  - Integrated antenna matching
  - Low-power Rx: 3.0 mA and Tx:2.9 mA (0dBm)
  - Wireless Protocols: Bluetooth LE, Matter, OpenThread, Zigbee

- Analog Peripherals

  - SAR ADC with up to 11 ENOB of resolution, 8 channels (P007-P009, P018-P022) with external
    reference support (VREFEXT on P017)
  - Analog voltage Comparator with 64 input reference levels
  - Temperature Sensor: ±2°C accuracy over the entire temperature range (-40°C to 125°C)

- Digital Interfaces

  - Up to 47 General Purpose I/Os across 2 voltage domains: P0xx (32x 1.8V),
    P1xx (8x 1.8V or 3.3V), P2xx (7x 1.8V)
  - USB FS interface (P106 D+, P107 D-)
  - QSPI FLASH/PSRAM SDR/DDR interface with XiP and decrypt-on-the-fly up to 64 MHz (P000-P006)
  - CAN-FD interface: 1-channel support, up to 1 Mbps in CAN mode and 8 Mbps in CAN-FD mode,
    with ECC support
  - 4x UARTs up to 6 Mbps, with 1 UART extended to support ISO7816
  - 3x SPI controllers with 32 bytes RX/TX FIFOs each and up to 32 MHz SPI clock
  - 3x I2C controllers with 32 deep RX/TX FIFOs each and up to 3.4 Mbps baud rate
  - 1x I3C interface up to 10.0 Mbps
  - PDM interface with hardware sample rate converter
  - I2S/PCM master/slave interface
  - 2x 3-axis capable Quadrature decoders
  - Keyboard scanner (16 columns x 8 rows)

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The EK-RA6B1 DEVKIT-PRO features an onboard `SEGGER J-Link`_ adapter that provides programming,
debugging, and serial communication.

Flashing
========

Before flashing, verify that your SEGGER J-Link installation includes support for the RA6B1,
as described in the kit manual.

Build a Zephyr application (e.g., Blinky):

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/blinky
      :board: ek_ra6b1/r7ka6b1bg4znq
      :goals: build flash

Debugging
=========

Once the board is connected to a PC via USB, a debugging session can be launched using the
onboard J-Link adapter in the standard way, for example by running ``west debug`` command.

References
**********

.. target-notes::

.. _SEGGER J-Link: https://www.segger.com/jlink-debug-probes.html
