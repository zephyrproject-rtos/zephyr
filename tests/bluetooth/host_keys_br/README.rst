.. _bluetooth_host_keys_br_tests:

Bluetooth Classic Link Key Tests
################################

Overview
********

This suite tests BR/EDR link key allocation when the two-entry key pool is full.
With oldest-key replacement enabled, allocation must preserve keys belonging to
connected BR/EDR peers, including peers awaiting disconnect cleanup, and reuse
the oldest unused entry. If no entry can be replaced, allocation must fail
without changing existing keys or deleting their settings.

The tests call the host key allocator directly. Connection enumeration is wrapped
to supply connection objects with controlled peer addresses and states. Settings
deletion is wrapped to check the deleted address and call count; it does not
access persistent storage. The settings runtime interface exercises the link key
restore handler.

Test Coverage
*************

* Return an existing key without eviction when the pool is full.
* Preserve a connected peer's key in either pool slot, including a connection
  that has not yet attached its link key pointer.
* Preserve keys in ``BT_CONN_DISCONNECTING`` and ``BT_CONN_DISCONNECT_COMPLETE``
  until normal disconnect processing releases them.
* Reject allocation when every entry belongs to a connected peer, leaving key
  contents, connection key pointers, and settings unchanged.
* Reject allocation with the same guarantees when every entry belongs to a peer
  awaiting disconnect cleanup.
* Replace the oldest unused key and delete only that peer's settings.
* Allow replacement of a disconnected peer's key even if its connection object
  still holds a key pointer.
* Reject allocation without changing existing keys when replacement is disabled.
* Return ``-ENOMEM`` from the settings restore handler when no slot is available.

Configurations
**************

``bluetooth.host_keys_br``
   Enables oldest-key replacement and Bluetooth settings.

``bluetooth.host_keys_br.no_overwrite``
   Disables oldest-key replacement and verifies full-pool allocation failure.

``bluetooth.host_keys_br.no_settings``
   Disables settings and runs the allocation and eviction tests without settings
   operations.

Building and Running
********************

The suite supports ``native_sim`` and ``qemu_x86``. It requires a configured
Zephyr build environment, but no Bluetooth controller, peer device, or Bumble
installation.

From the Zephyr repository directory, build and run all three configurations on
``native_sim``:

.. code-block:: shell

   west twister -T tests/bluetooth/host_keys_br -p native_sim --inline-logs

To run them on ``qemu_x86`` instead:

.. code-block:: shell

   west twister -T tests/bluetooth/host_keys_br -p qemu_x86 --inline-logs

Scope
*****

These are host key-management tests, not pairing or interoperability tests.
They do not exercise HCI connection transitions, disconnect cleanup races,
cross-transport key derivation (CTKD), or bond persistence across reboot.
