.. _bluetooth_setup:

Bluetooth
##########

To build any of the Bluetooth samples, follow the same steps as building
any other Zephyr application. Refer to the
:ref:`Getting Started Guide <getting_started>` for more information.

Many Bluetooth samples can be run on QEMU with support for external
Bluetooth Controllers. Refer to the :ref:`bluetooth_qemu` section for further
details.

Several of the bluetooth samples will build a Zephyr-based Controller that can
then be used with any external Host (including Zephyr running natively or
with QEMU), those are named accordingly with an "HCI" prefix in the
documentation and are prefixed with :literal:`hci_` in their folder names.

.. toctree::
   :maxdepth: 1
   :glob:

   **/*
