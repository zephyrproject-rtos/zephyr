.. zephyr:board:: mck_ra8t2

Overview
********

The MCK-RA8T2 is an Motor Control Kit for Renesas RA8T2 MCU Group which integrates multiple series of software-compatible
Arm®-based 32-bit cores that share a common set of Renesas peripherals to facilitate design scalability and efficient
platform-based product development.

The MCU in this series incorporates a high-performance Arm® Cortex®-M85 core running up to 1 GHz and Arm®
Cortex®-M33 core running up to 250 MHz with the following features:

- Up to 1 MB MRAM
- 2 MB SRAM (256 KB of CM85 TCM RAM, 128 KB CM33 TCM RAM, 1664 KB of user SRAM)
- Octal Serial Peripheral Interface (OSPI)
- Layer 3 Ethernet Switch Module (ESWM), USBFS, SD/MMC Host Interface
- Analog peripherals
- Security and safety features

MCK-RA8T2 kit includes the items below:

- RA8T2 CPU board (`MCB-RA8T2`_)
- Inverter board (`MCI-LV-1`_)
- Communication board (`MC-COM`_)
- Permanent magnet synchronous motors
- Accessories (cables, standoffs, etc.)

The specifications of the CPU board are shown below:

**MCU specifications**

- A high-performance RA8T2 MCU featuring an Arm® Cortex®-M85 core running up to 1 GHz and an Arm® Cortex®-M33 core
  running up to 250 MHz, offered in a 289-pin BGA package.
- MRAM/SRAM size: 1MB/2MB
- MCU input clock: 24MHz (Generate with external crystal oscillator)
- Power supply: DC 5V, select one way automatically from the below:

  - Power is supplied from compatible inverter board
  - Power is supplied from USB connector

**Connector**

- Inverter board connector (2 pair)
- USB connector for J-Link On-Board
- USB connector for RA8T2
- SCI connector for Renesas Motor Workbench communication
- Through hole for CAN communication
- 20 pin through hole for Arm debugger
- Pmod connectors (Type2A + Type3A/6A)
- User-controllable LED x6, Power LED x1,Ether CAT LED x8
- MCU reset switch
- Ether CAT connector
- MicroSD slot
- DSMIF

**Onboard debugger**

This product has the onboard debugger circuit, J-Link On-Board (hereinafter called “J-Link-OB”). When you write a
program, open the JP3 and connect the USB connector(CN13) on CPU board to PC with USB cable. J-Link-OB operates as debugger equivalent to J-Link.
If connecting from flash programing tool (e.g. J-Flash Lite by SEGGER), set the type of debugger (tool) to “J-Link”

Hardware
********
Detailed Hardware features for the RA8T2 MCU group can be found at:
- The RA8T2 MCU group: `RA8T2 Group User's Manual Hardware`_
- The MCB-RA8T2 board: `MCB-RA8T2 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

.. note::

   For using SDHC module on MCK-RA8T2, Connect microSD Card to microSD Socket (CN17)

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``mck_ra8t2`` board configuration can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Here is an example for the :zephyr:code-sample:`hello_world` application on CM85 core.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mck_ra8t2/r7ka8t2lfecac/cm85
   :goals: flash

Open a serial terminal, reset the board (push the reset switch S1), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.2.0-xxx-xxxxxxxxxxxxx *****
   Hello World! mck_ra8t2/r7ka8t2lfecac/cm85

Flashing
========

Program can be flashed to MCB-RA8T2 via the on-board SEGGER J-Link debugger.
SEGGER J-link's drivers are available at https://www.segger.com/downloads/jlink/

To flash the program to board

1. Connect to J-Link OB via USB port to host PC

2. Make sure J-Link OB jumper is in default configuration as describe in `MCB-RA8T2 - User's Manual`_

3. Execute west command

	.. code-block:: console

		west flash -r jlink

References
**********
- `MCK-RA8T2 Website`_
- `RA8T2 MCU group Website`_

.. _MCK-RA8T2 Website:
   https://www.renesas.com/en/design-resources/boards-kits/mck-ra8t2

.. _RA8T2 MCU group Website:
   https://www.renesas.com/en/products/ra8t2

.. _MCB-RA8T2 - User's Manual:
   https://www.renesas.com/en/document/mat/mcb-ra8t2-users-manual?r=25576094

.. _RA8T2 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/ra8t2-group-users-manual-hardware?r=25575951

.. _MCB-RA8T2:
   https://www.renesas.com/en/design-resources/boards-kits/mcb-ra8t2

.. _MCI-LV-1:
   https://www.renesas.com/en/design-resources/boards-kits/mci-lv-1

.. _MC-COM:
   https://www.renesas.com/en/design-resources/boards-kits/mc-com
