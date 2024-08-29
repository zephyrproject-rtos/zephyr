.. _bluetooth-samples:

Bluetooth samples
#################

To build any of the Bluetooth samples, follow the same steps as building
any other Zephyr application. Refer to :ref:`bluetooth-dev` for more information.

Many Bluetooth samples can be run on QEMU or :ref:`native_sim <native_sim>` with support for
external Bluetooth Controllers. Refer to the :ref:`bluetooth-hw-setup` section
for further details.

Several of the bluetooth samples will build a Zephyr-based Controller that can
then be used with any external Host (including Zephyr running natively or with
QEMU or ``native_sim``), those are named accordingly with an "HCI" prefix in the
documentation and are prefixed with :literal:`hci_` in their folder names.

.. note::
   If you want to run any bluetooth sample on the nRF5340 device (build using
   ``-DBOARD=nrf5340dk/nrf5340/cpuapp`` or
   ``-DBOARD=nrf5340dk/nrf5340/cpuapp/ns``) you must also build
   and program the corresponding sample for the nRF5340 network core
   :zephyr:code-sample:`bluetooth_hci_ipc` which implements the Bluetooth
   Low Energy controller.

.. note::
   The mutually-shared encryption key created during host-device paring may get
   old after many test iterations.  If this happens, subsequent host-device
   connections will fail. You can force a re-paring and new key to be created
   by removing the device from the associated devices list on the host.

.. toctree::
   :maxdepth: 1
   :glob:

   **/*
