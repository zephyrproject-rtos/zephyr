.. _bluetooth_classic_l2cap_connectionless_test:

Bluetooth Classic L2CAP Connectionless Channel Test
####################################################

Overview
********

This test suite verifies the Bluetooth Classic (BR/EDR) L2CAP Connectionless
data channel behavior using the Bluetooth Classic simulation test
infrastructure.

The test is executed on the Zephyr ``native_sim`` platform and uses the
`Bumble <https://github.com/google/bumble>`_ Bluetooth controller simulator.
Two simulated controllers are started and connected to two instances of the
same test binary via ``--bt-dev=127.0.0.1:<tcp_port>``.

The test binary is started twice:

* **Device 1 role**: runs the ``l2cap_connless_device1::...`` ztest suite.
  It performs BR/EDR inquiry, initiates a BR/EDR ACL connection to the peer,
  then sends and receives L2CAP connectionless channel frames.

* **Device 2 role**: runs the ``l2cap_connless_device2::...`` ztest suite.
  It waits for an incoming BR/EDR ACL connection, then sends and receives
  L2CAP connectionless channel frames.

Implementation notes
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

Test Scenarios
**************

Scenarios are defined in :file:`tests.yaml` and implemented by scripts in
:file:`tests_scripts/`. Each scenario runs as an independent Twister test case.

Available scenarios:

* ``bluetooth.classic.sim.l2cap_connectionless.basic_send_recv``

  * Script: :file:`tests_scripts/basic_send_recv.sh`
  * Runs ``test_01_basic_send_recv`` on both device roles.

* ``bluetooth.classic.sim.l2cap_connectionless.multiple_psm``

  * Script: :file:`tests_scripts/multiple_psm.sh`
  * Runs ``test_02_multiple_psm`` on both device roles.

* ``bluetooth.classic.sim.l2cap_connectionless.register_unregister_errors``

  * Script: :file:`tests_scripts/register_unregister_errors.sh`
  * Runs ``test_03_register_unregister_errors`` on both device roles.

What is tested
**************

* ``test_01_basic_send_recv``

  * BR/EDR inquiry and peer matching by BD_ADDR
  * BR/EDR ACL connection establishment
  * Single-PSM connectionless channel: Device 1 sends to Device 2, Device 2
    replies to Device 1
  * Verification of received PSM value

* ``test_02_multiple_psm``

  * Simultaneous registration of two distinct PSMs (``0x1001`` and ``0x1003``)
    plus a wildcard callback (PSM ``0``)
  * Connectionless data delivery to the correct per-PSM callback on both sides
  * Wildcard callback fires for every received frame regardless of PSM

* ``test_03_register_unregister_errors``

  * Double-register of the same callback returns ``-EEXIST``
  * Wildcard callback (PSM 0) receives frames for any PSM alongside the
    PSM-specific callback
  * Unregister of an already-unregistered callback returns ``-ENOENT``
  * Unregister of a NULL callback returns ``-EINVAL``

Requirements
************

* Host environment capable of running ``native_sim`` tests
* Bumble available for the Bluetooth Classic controller simulation

Building and Running
********************

This test is intended to be run via Twister on ``native_sim``.

Build and run with Twister
==========================

Run all scenarios:

.. code-block:: console

   west twister -T tests/bluetooth/classic/sim/l2cap_connectless

Run a single scenario:

.. code-block:: console

   west twister -T tests/bluetooth/classic/sim/l2cap_connectless \
       -s bluetooth.classic.sim.l2cap_connectionless.basic_send_recv

Direct script execution
=======================

Twister uses the script harness defined in :file:`tests.yaml` and runs the
scripts listed in ``harness_config/tests_scripts``.

For example:

.. code-block:: console

   ./tests/bluetooth/classic/sim/l2cap_connectless/tests_scripts/basic_send_recv.sh
   ./tests/bluetooth/classic/sim/l2cap_connectless/tests_scripts/multiple_psm.sh
   ./tests/bluetooth/classic/sim/l2cap_connectless/tests_scripts/register_unregister_errors.sh

Each script:

* allocates two TCP ports,
* starts two Bumble controllers (one per port, with fixed BD_ADDR values),
* launches the test binary twice:

  * Device 1 instance (``-test=l2cap_connless_device1::...``)
  * Device 2 instance (``-test=l2cap_connless_device2::...``)

If you see "west: command not found", activate the Zephyr virtual environment:

.. code-block:: console

   source ~/zephyrproject/.venv/bin/activate

Configuration Options
*********************

See :file:`prj.conf` for the default configuration.
Additional logging configuration is available in :file:`Kconfig`.
The ``CONFIG_BT_L2CAP_CONNLESS`` Kconfig option must be enabled (set via
``extra_configs`` in :file:`tests.yaml`).
