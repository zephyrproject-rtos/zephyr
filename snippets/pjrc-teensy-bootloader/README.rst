.. _snippet-pjrc-teensy-bootloader:

PJRC Teensy Bootloader (pjrc-teensy-bootloader)
###############################################

.. code-block:: console

   west build -S pjrc-teensy-bootloader [...]

Overview
********

This snippet makes the nessecary configurations to allow flashing of Teensy 4
based boards without triggering the reset pin. This is done via a CDC ACM UART.

Requirements
************

A board that supports flashing via the teensy cli tools.

A devicetree node with node label ``zephyr_udc0`` that points to an enabled USB
device node with driver support. This should look roughly like this in
:ref:`your devicetree <get-devicetree-outputs>`:

.. code-block:: DTS

   usb1: zephyr_udc0: usbd@402e0000 {
      compatible = "nxp,ehci";
        /* ... */
   };

The Teensy 4.0 and 4.1 boards have this node by default, but others might not.
