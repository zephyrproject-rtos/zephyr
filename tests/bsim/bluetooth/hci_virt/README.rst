.. Copyright 2026 Xiaomi Corporation
..
.. SPDX-License-Identifier: Apache-2.0

Bluetooth Host test over Unix HCI sockets
#########################################

This test runs a central and a peripheral using separate Zephyr Hosts and virtual
controllers. Every executable targets ``nrf52_bsim/native``. The Host build uses
``bsim_host_only.overlay`` to select the existing ``zephyr,bt-hci-userchan`` driver and
disable HCI UART. ``CONFIG_BT_LL_SW_SPLIT=n`` excludes the local controller.

The test starts five processes:

* A central Host connected to its controller with ``--bt-dev=<central socket>``.
* A peripheral Host connected to its controller with ``--bt-dev=<peripheral socket>``.
* Two instances of :zephyr:code-sample:`bluetooth_hci_virt`.
* The BabbleSim PHY.

All four devices join the same PHY simulation and use ``-mro=1000`` to synchronize
their clocks. The Hosts do not perform radio operations. Only the two
controllers transmit and receive radio packets. All devices use ``-sync_preboot`` to
wait for PHY time zero before booting. The PHY handles these waits in device-number
order, so the controllers use device numbers 0 and 1 and the Hosts use 2 and 3.
Each controller binds and listens on its socket before its next PHY request,
allowing the Hosts to connect without a startup delay.

Test scenario
*************

``bluetooth.hci_virt.gatt`` exercises HCI command, event, and ACL traffic over the sockets.
The peripheral advertises, the central scans for its public address, and the central
initiates a connection. Both Hosts check their connection roles.

On the same connection, the central discovers the test characteristic, reads its initial
value, writes a binary payload, and reads the updated value. It then subscribes to
notifications. The peripheral validates the write payload, waits for the CCC write, and
sends five notifications with the updated value. The central validates both read responses,
every notification payload, and the notification count before disconnecting. Both Hosts
check the disconnection event.

Running
*******

Configure ``BSIM_OUT_PATH`` and ``BSIM_COMPONENTS_PATH`` as described in
:ref:`nrf52bsim_build_and_run`. The test requires the ``bsim_multi_test`` fixture,
which is enabled in the BabbleSim CI job. To run it locally:

.. code-block:: console

   west twister -p nrf52_bsim/native -T tests/bsim/bluetooth/hci_virt \
     --fixture bsim_multi_test -i

To build the Host directly:

.. zephyr-app-commands::
   :zephyr-app: tests/bsim/bluetooth/hci_virt
   :board: nrf52_bsim/native
   :build-dir: build/hci_virt_host
   :goals: build
   :compact:

The Host application's CMake configuration loads ``bsim_host_only.overlay`` by default.
