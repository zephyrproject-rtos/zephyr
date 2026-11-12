.. zephyr:code-sample:: usb-ccid
   :name: USB CCID reader backend
   :relevant-api: usbd_ccid

   Expose a simulated T=0 card through the USB CCID class.

Overview
********

This sample implements the CCID backend API without a physical smartcard,
radio transport or external application library. The USB host sees one reader
with a present card. Power-on returns a minimal T=0 ATR. The simulated card
accepts SELECT instructions with status ``9000`` and returns ``6D00`` for other
instructions; it does not implement a card application or credential storage.

The card parameters and reader capabilities live in the backend. The class
only handles USB CCID framing, lifecycle and dispatch. Use the backend API to
connect a real card transport or a different simulated card.

Requirements
************

A board supported by Zephyr's USB device stack, with a ``zephyr_udc0`` node
and enough endpoints for a bulk-IN, bulk-OUT and interrupt-IN endpoint.

Building and running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/ccid
   :board: nrf52840dk/nrf52840
   :goals: build flash

Connect the board's device USB port to a host with a CCID/PCSC driver. A reader
named by the host driver appears, and its card can be powered on and accessed
using APDU commands. For a product, configure its own USB identifiers as
described in :ref:`usbd_vid_pid`.
