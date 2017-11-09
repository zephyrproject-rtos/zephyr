.. _bluetooth_setup:

Bluetooth
##########

To build any of the Bluetooth samples, follow the same steps as building
any other Zephyr application. Refer to the
:ref:`Getting Started Guide <getting_started>` for more information.

When building for qemu (e.g. BOARD=qemu_x86 or BOARD=qemu_cortex_m3),
the setup assumes that the host's Bluetooth controller is connected to
the second QEMU serial line through a Unix socket (QEMU option -serial
unix:/tmp/bt-server-bredr). This option is automatically added to the
qemu command line whenever building for a qemu target and Bluetooth
support has been enabled.

On the host side BlueZ allows to "connect" Bluetooth controller through a
so-called user channel. Use the btproxy tool for that:

Note that before calling ``btproxy`` make sure that Bluetooth controller is
down.

.. code-block:: console

   $ sudo tools/btproxy -u
   Listening on /tmp/bt-server-bredr

Running the application in QEMU will connect the second serial line to
``bt-server-bredr`` Unix socket. When Bluetooth (CONFIG_BT) and Bluetooth
HCI UART driver (CONFIG_BT_H4) are enabled, the Bluetooth driver
registers with the system.

From now on Bluetooth may be used by the application. To run applications in
the QEMU emulation environment, type:

.. code-block:: console

   $ make run

.. toctree::
   :maxdepth: 1
   :glob:

   **/*
