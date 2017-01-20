.. _bluetooth_setup:

Bluetooth
##########

To build any of the Bluetooth samples, follow the instructions below:

.. code-block:: console

   $ make -C samples/bluetooth/<app>

Host Bluetooth controller is connected to the second QEMU serial line through a
Unix socket (QEMU option -serial unix:/tmp/bt-server-bredr).  This option is
already added to Qemu through QEMU_EXTRA_FLAGS in Makefile.

On the host side BlueZ allows to "connect" Bluetooth controller through a
so-called user channel. Use the btproxy tool for that:

Note that before calling ``btproxy`` make sure that Bluetooth controller is
down.

.. code-block:: console

   $ sudo tools/btproxy -u
   Listening on /tmp/bt-server-bredr

Running the application in Qemu will connect the second serial line to
``bt-server-bredr`` Unix socket. When Bluetooth (CONFIG_BLUETOOTH) and Bluetooth
HCI UART driver (CONFIG_BLUETOOTH_H4) are enabled, the Bluetooth driver
registers with the system.

From now on Bluetooth may be used by the application. To run applications in
the Qemu emulation environment, type:

.. code-block:: console

   $ make run

.. toctree::
   :maxdepth: 1
   :glob:

   **/*
