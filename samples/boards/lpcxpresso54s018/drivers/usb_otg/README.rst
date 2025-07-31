.. _usb_otg_sample:

USB OTG Sample
##############

Overview
********

This sample demonstrates USB On-The-Go (OTG) functionality on the LPC54S018 board.
It shows automatic role switching between USB host and device modes based on the
ID pin state.

The sample implements:

- Automatic role detection via ID pin
- USB host mode when ID pin is grounded (OTG cable)
- USB device mode when ID pin is floating (standard cable)
- VBUS control for powering devices in host mode
- CDC ACM device functionality in device mode
- Device enumeration in host mode

Requirements
************

- LPC54S018 board
- USB OTG cable (for host mode)
- Standard USB cable (for device mode)
- USB device to connect when in host mode

Hardware Setup
**************

The sample uses the following pins:

- P1.11 - USB ID pin (pulled up internally)
- P1.6  - VBUS enable (GPIO output)
- USB1 connector for both host and device modes

Building and Running
********************

Build and flash this sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/usb_otg
   :board: lpcxpresso54s018
   :goals: build flash
   :compact:

Testing Host Mode
=================

1. Connect a USB OTG cable to the USB1 port
2. The ID pin will be grounded by the OTG cable
3. The board will enter USB host mode
4. Connect a USB device (e.g., flash drive, keyboard)
5. The device will be enumerated and information displayed

Testing Device Mode
===================

1. Connect a standard USB cable to the USB1 port
2. The ID pin will be floating (pulled high)
3. The board will enter USB device mode
4. The board will enumerate as a CDC ACM device
5. Connect to the virtual COM port to see messages

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0 ***
   [00:00:00.010,000] <inf> usb_otg_sample: USB OTG sample application
   [00:00:00.020,000] <inf> usb_otg_sample: USB OTG initialized - connect USB cable with ID pin
   [00:00:00.020,000] <inf> usb_otg_sample: ID pin LOW = Host mode, ID pin HIGH = Device mode
   [00:00:01.000,000] <inf> otg_lpc: USB OTG role change: none -> host
   [00:00:01.000,000] <inf> usb_otg_sample: USB OTG event: 3, role: HOST
   [00:00:01.010,000] <inf> usb_otg_sample: USB host mode enabled
   [00:00:05.123,000] <inf> usb_otg_sample: USB device connected in host mode
   [00:00:05.123,000] <inf> usb_otg_sample:   VID: 0x0781, PID: 0x5567

OTG Cable Detection
===================

The USB OTG role is determined by the ID pin:

- OTG cable: ID pin connected to ground → Host mode
- Standard cable: ID pin floating → Device mode

The role switching is automatic and happens when the cable is changed.

References
**********

- :ref:`usb_api`
- `USB OTG Specification`_
- `LPC54S018 Reference Manual`_

.. _USB OTG Specification:
   https://www.usb.org/sites/default/files/otg_1_3.zip

.. _LPC54S018 Reference Manual:
   https://www.nxp.com/docs/en/reference-manual/LPC54S01XJ2J4RM.pdf