.. zephyr:board:: ek_rx74m

Overview
********

The EK-RX74M is an Evaluation Kit for the Renesas RX74M MCU Group, part of the RX700
Series. It enables users to seamlessly evaluate the features of the RX74M MCU group
and develop embedded systems applications.

The MCU in this series is based on the RXv3e CPU core running up to 600 MHz with the
following features:

- 1 MB Code MRAM
- 1.5 MB RAM
- Built-in encryption module
- CAN FD (Controller Area Network with Flexible Data Rate)
- Δ-Σ Modulator Interface (DSMIF)
- Layer 3 Ethernet Switch Module (ESWM) / EtherCAT® Slave Controller (ESC), 2 Ethernet ports
- USB High Speed and USB Full Speed, host and function
- SD/MMC Host Interface (SDHI)
- eXpanded SPI (XSPI) interface for external OctaFlash and OctalRAM
- RS-485

**MCU Native Pin Access**

- 600 MHz RX74M MCU (R5K574MCB4AE) in 320-pin, LFBGA package
- Native pin access through 20 x 2-pin x 3-piece male headers
- MCU and USB current measurement points for current consumption measurement
- 24.000 MHz crystal oscillator as main clock
- 32.768 kHz crystal oscillator as sub clock
- 512 Mbit External OctaFlash and 256 Mbit External OctalRAM
  (present in the MCU Native Pin Access area of the EK-RX74M board)
- Two Δ-Σ Modulator Interface (DSMIF) connectors (not populated, present in the MCU
  Native Pin Access area of the EK-RX74M board)

**System Control and Ecosystem Access**

- Five 5 V input sources

  - USB (Debug, High Speed, Full Speed, Serial)
  - External power supply (using surface mount clamp test points and power input vias)

- Two Debug modes

  - Debug on-board (E2 emulator On Board, FINE interface)
  - Debug in (JTAG, SWD, FINE interface)

- User LEDs and switches

  - Three User LEDs (red, blue, green)
  - Power LED (white) indicating availability of regulated power
  - Debug LED (yellow) indicating the debug connection
  - Two User switches
  - One Reset switch

- Five most popular ecosystems expansions

  - Two Seeed Grove system (I2C/Analog) connectors (not populated)
  - One SparkFun Qwiic connector (not populated)
  - Two Digilent Pmod (UART/SPI/I2C) connectors
  - Arduino (Uno R3) connector
  - MikroElektronika mikroBUS connector

- USB serial converter interface
- USB Full Speed host and function (present in the Special Feature Access area of the
  EK-RX74M board)

**Special Feature Access**

- CAN FD transceiver and interface connector
- RS-485 transceiver and interface connector (connector not populated)
- Ethernet connector (2 ports, MII/GMII/RMII interface) or EtherCAT®
- USB High Speed host and function
- microSD card slot

Hardware
********

Detailed hardware features can be found at:

- RX74M MCU: RX74M Group User's Manual Hardware
- EK-RX74M board: EK-RX74M - User's Manual

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
**************************

.. zephyr:board-supported-runners::

Applications for the ``ek_rx74m`` board configuration can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ek_rx74m
   :goals: flash

Open a serial terminal, reset the board, and you should see the following
message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.4.0-xxx-xxxxxxxxxxxxx *****
   Hello World! ek_rx74m

Flashing
========

Program can be flashed to EK-RX74M via the on-board E2 emulator On Board (E2OB)
circuit using Renesas Flash Programmer (RFP) command line interface (rfp-cli).
Renesas Flash Programmer is available at
https://www.renesas.com/en/software-tool/renesas-flash-programmer-programming-gui

To flash the program to board

1. Connect the USB DEBUG1 connector to a host PC. Set the DIP SW (SW6) to Debug
   on-board mode as described in EK-RX74M - User's Manual.

2. Execute west command

   .. code-block:: console

      west flash -r rfp

References
**********

- EK-RX74M Website
- RX74M MCU group Website
- RX74M Group User's Manual Hardware
- EK-RX74M - User's Manual
