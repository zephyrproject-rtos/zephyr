.. _bluetooth_classic_l2cap_connectionless_test:

Bluetooth Classic L2CAP Connectionless Channel Test
####################################################

Overview
********

This test suite verifies the Bluetooth Classic (BR/EDR) L2CAP Connectionless
data channel behavior using the Bumble Bluetooth controller simulator sidecar.

The test is executed on the Zephyr ``native_sim`` platform and uses the
`Bumble <https://github.com/google/bumble>`_ Bluetooth controller simulator.
Two simulated controllers are started by Twister's Bumble sidecar and connected
to two instances of the same test binary via ``--bt-dev=127.0.0.1:<tcp_port>``.
Each instance receives a ``--peer_bd_address`` argument so the two roles can
locate each other.

The test binary is started twice:

* **Device 1 role** (``l2cap_connless_device1::...`` ztest suite): performs
  BR/EDR inquiry, initiates a BR/EDR ACL connection to the peer, then sends
  and receives L2CAP connectionless channel frames.

* **Device 2 role** (``l2cap_connless_device2::...`` ztest suite): waits for
  an incoming BR/EDR ACL connection, then sends and receives L2CAP
  connectionless channel frames.

Implementation Notes
********************

* The peer BD_ADDR is passed on the command line and parsed by a
  :c:macro:`NATIVE_TASK` registration in :file:`src/test_main.c`.
* Both roles register connectionless callbacks via
  :c:func:`bt_l2cap_br_connless_register`.
* Outgoing data is sent via :c:func:`bt_l2cap_br_connless_send` with headroom
  reserved via :c:macro:`BT_L2CAP_CONNLESS_RESERVE`.
* The test PSMs ``0x1001`` and ``0x1003`` comply with the L2CAP PSM encoding
  rule (least-significant bit of the most-significant octet is 0, and the
  least-significant bit of all other octets is 1).
* A wildcard callback registered with PSM ``0`` receives frames for any PSM.
* All three scenarios enable ``CONFIG_BT_L2CAP_CONNLESS=y`` via
  ``extra_configs`` in :file:`tests.yaml`.

Test Cases
**********

``bluetooth.classic.sim.l2cap_connectionless.basic_send_recv``
  Runs ``test_01_basic_send_recv`` on both device roles.

  * BR/EDR inquiry and peer matching by BD_ADDR
  * BR/EDR ACL connection establishment
  * Single-PSM connectionless channel: Device 1 sends to Device 2, Device 2
    replies to Device 1
  * Verification of received PSM value

``bluetooth.classic.sim.l2cap_connectionless.multiple_psm``
  Runs ``test_02_multiple_psm`` on both device roles.

  * Simultaneous registration of two distinct PSMs (``0x1001`` and ``0x1003``)
    plus a wildcard callback (PSM ``0``)
  * Connectionless data delivery to the correct per-PSM callback on both sides
  * Wildcard callback fires for every received frame regardless of PSM

``bluetooth.classic.sim.l2cap_connectionless.register_unregister_errors``
  Runs ``test_03_register_unregister_errors`` on both device roles.

  * Double-register of the same callback returns ``-EEXIST``
  * Wildcard callback (PSM 0) receives frames for any PSM alongside the
    PSM-specific callback
  * Unregister of an already-unregistered callback returns ``-ENOENT``
  * Unregister of a NULL callback returns ``-EINVAL``

Requirements
************

* Host environment capable of running ``native_sim`` tests.
* Python package ``bumble`` installed (used by Twister's Bumble sidecar).

Building and Running
********************

This test is intended to be run via Twister on ``native_sim``.

Run all scenarios:

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/l2cap_connectless

Run a single scenario:

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/l2cap_connectless \
       -s bluetooth.classic.sim.l2cap_connectionless.basic_send_recv

Twister invokes the ``bumble`` sidecar defined in :file:`tests.yaml`. The
sidecar starts two Bumble controllers with fixed BD_ADDR values
(``00:00:01:00:00:01`` and ``00:00:01:00:00:02``) and passes the matching
``--peer_bd_address`` and ``-test`` arguments to each binary instance
automatically.

If you see "west: command not found", activate the Zephyr virtual environment:

.. code-block:: console

   source ~/zephyrproject/.venv/bin/activate

Refer to :ref:`getting_started` for details.

Configuration Options
*********************

See :file:`prj.conf` for the default configuration.

Additional logging configuration is available in :file:`Kconfig`.
The ``CONFIG_BT_L2CAP_CONNLESS`` Kconfig option must be enabled (set via
``extra_configs`` in :file:`tests.yaml`).
