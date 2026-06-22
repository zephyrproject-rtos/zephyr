.. zephyr:board:: mcb_rx14t

Overview
********

MCB-RX14T is a motor control evaluation kit. By using this product, motor control with MCK-RX14T can
be performed easily.

MCK-RX14T has characteristics shown below

- Supports Brushless DC motor.
- Supports 1-/2-/3-shunt current detection.
- Supports Motor Control Development Support Tool (Renesas Motor Workbench).
- Provides overcurrent protection function using overcurrent detection circuit.

The key features of the MCB-RX14T are categorized in two groups (consistent with the architecture
of the board) as follows:

**MCU Native Pin Access**

The MCB-RX14T includes:

- R5F514T5AMFM MCU (referred to as RX MCU)

  - Max 48 MHz, 32-bit RX CPU (RXv2)
  - 128 KB ROM, 12 KB RAM
  - 64-pin, LFQFP package

**System Control and Ecosystem Access**

- DC 5V,3.3V (selectable with jumper pin) input sources
- Select one way automatically from the below
   - Power is supplied from compatible inverter board
   - Power is supplied from USB connector

- E2OB (Onboard debugger circuit)
- User LEDs and switches

   - 5 User LEDs
   - Power LED indicating availability of regulated power
   - MCU reset switch

- Connectors

  - Inverter board connector
  - USB connector for E2OB
  - SCI connector for Renesas Motor Workbench communication
  - Through hole for SPI communication
  - Pmod connectors

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``mcb_rx14t`` board can be built, flashed, and debugged using standard
Zephyr workflows. Refer to :ref:`build_an_application` and :ref:`application_run` for more details.

Flashing
========

The program can be flashed to MCB-RX14T using the **E2 Lite debugger** by
connecting the board's 14-pin debug connector to the host PC.
Here’s an example for building and flashing the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mcb_rx14t
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
