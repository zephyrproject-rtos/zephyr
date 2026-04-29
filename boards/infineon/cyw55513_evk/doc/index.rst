.. _cyw55513_evk:

AIROC CYW55513 Evaluation Kit
==============================

Overview
*********

The AIROC CYW55513 Evaluation Kit is designed for evaluating and developing
applications using the Infineon CYW55513 Wi-Fi & Bluetooth combo chipset.
The CYW55513 is a low-power, single chip device that supports:

- **Wi-Fi**: 802.11a/b/g/n/ac/ax (tri-band), single-stream
- **Bluetooth**: 5.4 BR/EDR/LE with up to 143 Mbps PHY data rate (Wi-Fi), 3 Mbps (Bluetooth)

The evaluation kit is available in two module configurations:

1. **CYW55513IUBG_SM** - Discrete module variant
2. **CYW55513_MURATA_2FY** - Murata Type 2FY small form factor module

Hardware
*********

The CYW55513 EVK features:

- AIROC CYW55513 combo chipset (Wi-Fi and Bluetooth)
- On-board debugging interface (JTAG/SWD)
- GPIO expansion headers
- UART interface for console output
- LEDs and push buttons for user interaction
- USB connector for power and debugging

Supported Features
*******************

The board configuration supports the following hardware features:

- :abbr:`GPIO (General Purpose Input/Output)`
- :abbr:`UART (Universal Asynchronous Receiver/Transmitter)`
- :abbr:`ADC (Analog-to-Digital Converter)`
- :abbr:`WDT (Watchdog Timer)`
- :abbr:`Bluetooth (Bluetooth Low Energy)`
- :abbr:`Wi-Fi (802.11 Wireless)`

Connections and IOs
********************

Console
=======

The UART interface is used for console communication:

- **TX Pin**: GPIO0.5
- **RX Pin**: GPIO0.4
- **Baud Rate**: 115200

LEDs
====

- LED0: GPIO0.13 (Active High)
- LED1: GPIO0.14 (Active High)

Buttons
=======

- Button0 (SW0): GPIO0.12 (Active Low)

Debugging
*********

The board supports debugging via:

- **OpenOCD**: For ARM Cortex-M targets
- **J-Link**: For J-Link compatible debuggers

To debug using OpenOCD:

.. code-block:: console

   west debug --runner openocd

To debug using J-Link:

.. code-block:: console

   west debug --runner jlink

Programming and Debugging
**************************

Building
========

To build a sample application for the CYW55513 EVK:

.. code-block:: console

   west build -b cyw55513_evk/cyw55513iubg_sm samples/hello_world

Or for the Murata 2FY variant:

.. code-block:: console

   west build -b cyw55513_evk/cyw55513_murata_2fy samples/hello_world

Flashing
========

To flash the built application to the board:

.. code-block:: console

   west flash --runner openocd

Or with J-Link:

.. code-block:: console

   west flash --runner jlink

References
***********

- `Infineon AIROC CYW55513 <https://www.infineon.com/cms/en/product/wireless-connectivity/airoc/airoc-wi-fi-bt-combos/airoc-cyw55513/>`_
- `Murata Type 2FY Module <https://www.murata.com/en-us/products/connectivitymodule/wi-fi-bluetooth/overview/lineup/type2fy>`_
