.. _external_module_cannectivity:

CANnectivity USB to CAN adapter firmware
########################################

Introduction
************

`CANnectivity`_ is an open source firmware for Universal Serial Bus (USB) to Controller Area Network
(CAN) adapters.

The firmware implements the Geschwister Schneider USB/CAN device protocol (often referred to as
"gs_usb"). This protocol is supported by the Linux kernel SocketCAN `gs_usb driver`_, by
`python-can`_, and by many other software packages.

The firmware, which is based on Zephyr RTOS, allows turning your favorite microcontroller
development board into a full-fledged USB to CAN adapter.

CANnectivity is licensed under the Apache-2.0 license.

Usage with Zephyr
*****************

The CANnectivity firmware repository is a Zephyr :ref:`module <modules>` which allows for reuse of
its components (i.e. the "gs_usb" protocol implementation) outside of the CANnectivity firmware
application.

To pull in CANnectivity as a Zephyr module, either add it as a West project in the ``west.yaml``
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/cannectivity.yaml``) file
with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: cannectivity
         url: https://github.com/CANnectivity/cannectivity.git
         revision: main
         path: custom/cannectivity # adjust the path as needed

Once CANnectivity is added as a Zephyr module, the "gs_usb" implementation can be reused outside of
the CANnectivity firmware application by including its header:

.. code-block:: c

   #include <cannectivity/usb/class/gs_usb.h>

Please see the header file for the API details.

.. _CANnectivity:
   https://github.com/CANnectivity/cannectivity

.. _gs_usb driver:
   https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/net/can/usb/gs_usb.c

.. _python-can:
   https://python-can.readthedocs.io/en/stable/interfaces/gs_usb.html
