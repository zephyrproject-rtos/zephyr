.. Copyright 2026 Xiaomi Corporation
..
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:code-sample:: bluetooth_hci_virt
   :name: Bluetooth HCI over a Unix socket
   :relevant-api: hci_raw

   Expose a simulated Bluetooth controller through a Unix domain socket.

Overview
********

This sample runs the Zephyr Bluetooth controller on :ref:`nrf52_bsim` and exposes its HCI
interface as an H:4 byte stream over a Unix domain socket. A host application can connect
without simulating a UART. Radio activity uses the BabbleSim PHY.

The transport accepts one host at a time. After a disconnect, it discards an incomplete
packet and accepts a new connection. The host must reset and initialize the controller
after reconnecting. A host that cannot receive a complete packet without blocking is
disconnected.

The receive thread polls the socket every millisecond of simulated time. You can use
``-mro=1000`` to limit the PHY resynchronization offset to one millisecond when
coordinating the controller with an external Host.

An optional BTSnoop file records H:4 packets directly in the controller. Timestamps use
simulation time in microseconds, offset by the BTSnoop epoch.

Requirements
************

* A Linux host with support for 32-bit native applications.
* BabbleSim, configured as described in :ref:`nrf52bsim_build_and_run`, with
  ``BSIM_OUT_PATH`` and ``BSIM_COMPONENTS_PATH`` set.

Building and running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/hci_virt
   :board: nrf52_bsim/native
   :goals: build
   :compact:

For an external Host running in real time, run the PHY, controller, and handbrake
device with the same simulation identifier:

.. code-block:: console

   "${BSIM_OUT_PATH}/bin/bs_2G4_phy_v1" -s=hci_virt -D=2 &
   ./build/zephyr/zephyr.exe -s=hci_virt -d=0 -mro=1000 -hci-socket=/tmp/hci0.sock &
   "${BSIM_OUT_PATH}/bin/bs_device_handbrake" -s=hci_virt -d=1 -r=1 &

The handbrake limits the simulation to approximately real-time speed. Without this,
simulation time advances as fast as the workstation allows, including between commands
from the Host. The ``-mro`` option controls how often the device synchronizes with the PHY.
See :ref:`About time in BabbleSim <nrf52bsim_time>` for details. If the Host coordinates
simulated time itself, omit the handbrake and set ``-D`` to the number of devices
participating in the PHY simulation.

Use a unique socket path and device number for each controller. Socket paths must fit
in the host's Unix socket address structure. Existing socket paths are removed at startup;
HCI socket and BTSnoop file paths must differ. You can stop the controller with :kbd:`Ctrl+C`.

Runtime options
***************

``-hci-socket=<path>``
   HCI transport socket path. Defaults to ``/tmp/hci0.sock``.

``-bd-addr=<AA:BB:CC:DD:EE:FF>``
   Set the public Bluetooth address before enabling the controller. The all-zero address
   is rejected. Omit this option to use the controller's default address.

``-btsnoop-file=<path>``
   Write BTSnoop capture to a file. Disabled by default. The parent directory must exist.
   An existing file is overwritten. File open or write failures terminate the controller.

Testing
*******

Run the central/peripheral tests with independent Zephyr Hosts over Unix HCI sockets:

.. code-block:: console

   west twister -p nrf52_bsim/native -T tests/bsim/bluetooth/hci_virt -i

Twister builds this controller sample and a separate Host-only application for
``nrf52_bsim/native``. The tests cover scanning, connection and disconnection, GATT
read/write operations, and notifications. See
:zephyr_file:`tests/bsim/bluetooth/hci_virt/README.rst` for the topology and test details.
