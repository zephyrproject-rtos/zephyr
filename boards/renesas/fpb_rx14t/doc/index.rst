.. zephyr:board:: fpb_rx14t

Overview
********

The FPB-RX14T, a Fast Prototyping Board for the RX14T MCU, enables users to seamlessly evaluate the
features of the RX14T MCU and develop embedded systems applications. Users can use on-board features
along with their choice of popular ecosystem add-on modules to bring their big ideas to life

The key features of the FPB-RX14T are categorized in two groups (consistent with the architecture
of the board) as follows:

**MCU Native Pin Access**

The FPB-RX14T includes:

- R5F514T5AMFM MCU (referred to as RX MCU)

  - Max 48 MHz, 32-bit RX CPU (RXv2)
  - 128 KB ROM, 12 KB RAM
  - 64-pin, LFQFP package

- Native pin access through 2 x 32-pin breakout pin headers (not fitted)
- RX MCU current measurement point for precision current consumption measurement
- RX MCU on-chip oscillators as clock

**System Control and Ecosystem Access**

- Two 5 V input sources

   - USB
   - External power supply (using 2-pin header [not fitted])

- On-board debugger / programmer (E2 emulator On Board (referred as E2OB, FINE Interface))
- User LEDs and switches

   - Two User LEDs (green)
   - Power LED (green) indicating availability of regulated power
   - Debug LED (yellow) indicating the debug connection
   - One User switch
   - One Reset switch

- Two popular ecosystem expansions

  - Two Digilent Pmod (Type-2A [expanded SPI], Type-3A [expanded UART] and Type-6A [expanded
    I2C]) connectors
  - Arduino Uno R3 (referred to as Arduino) connector

- SCI Interface Connector (not fitted)

  - Supports UART connection
  - Supports connection with the MC-COM Renesas Flexible Motor Control Communication Board

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``fpb_rx14t`` board can be built, flashed, and debugged using standard
Zephyr workflows. Refer to :ref:`build_an_application` and :ref:`application_run` for more details.

Flashing
========

The program can be flashed to FPB-RX14T using the **E2 Lite debugger** by
connecting the board's 14-pin debug connector to the host PC.
Here’s an example for building and flashing the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: fpb_rx14t
   :goals: build flash

Debugging
=========
You can use `Renesas Debug extension`_ on Visual Studio Code for a visual debug interface.
The configuration for launch.json is as below.

.. code-block:: json

  {
    "version": "0.2.0",
    "configurations": [
        {
            "type": "renesas-hardware",
            "request": "launch",
            "name": "RX14T Renesas Debugging E2lite",
            "target": {
                "deviceFamily": "RX",
                "device": "R5F514T5",
                "debuggerType": "E2LITE"
                "serverParameters": [
                    "-uUseFine=", "1",
                    "-w=", "0",
                ],
            }
        }
    ]
  }

References
**********
.. _Renesas Debug extension:
   https://marketplace.visualstudio.com/items?itemName=RenesasElectronicsCorporation.renesas-debug
