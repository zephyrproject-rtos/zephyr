.. SPDX-FileCopyrightText: Copyright 2026 NXP
.. SPDX-License-Identifier: Apache-2.0

.. _bluetooth_host_br_discovery_tests:

BR/EDR discovery completion
###########################

Overview
********

These host tests inject HCI event payloads directly into the discovery event
handlers and wrap command allocation and sending to verify Bluetooth Classic
discovery completion.

The devicetree overlay removes the board's ``zephyr,bt-hci`` chosen property so
the host does not reference a controller device.

Fixtures use the public discovery start and stop APIs, without directly
resetting discovery state.

Test Coverage
*************

The test suite covers:

* Completion with no discovery results.
* Completion without name resolution when an EIR name is available.
* Completion after successful or failed remote name resolution.
* Serial name resolution for multiple devices before reporting completion.
* Ignoring late name events without modifying application result storage,
  generating additional callbacks, or sending additional HCI commands.
* Starting another discovery from a timeout callback without the old completion
  handler clearing the new session.
* Delivering the original results to all timeout listeners even when an earlier
  listener starts another discovery.

Requirements
************

* A Zephyr development environment capable of building and running tests on
  ``native_sim`` or ``qemu_x86``.
* No Bluetooth controller, peer device, or Bumble installation is required.

Building and Running
********************

Run the suite with Twister from the Zephyr repository root.

For ``native_sim``:

.. code-block:: console

   west twister -p native_sim -T tests/bluetooth/host_br_discovery

For ``qemu_x86``:

.. code-block:: console

   west twister -p qemu_x86 -T tests/bluetooth/host_br_discovery

See :file:`tests.yaml` for the supported platforms and :file:`prj.conf` for the
default configuration.
