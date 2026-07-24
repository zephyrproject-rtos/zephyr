.. _bluetooth_classic_sim_l2cap_test:

Bluetooth Classic L2CAP Test
############################

Overview
********

This test suite verifies Bluetooth Classic BR/EDR L2CAP connection establishment and
data path behavior using the Bluetooth Classic simulation test infrastructure.

The test is executed on the Zephyr ``native_sim`` platform and uses the
`Bumble <https://github.com/google/bumble>`_ Bluetooth controller simulator.
Two simulated controllers are started and connected to two instances of the same
test binary via ``--bt-dev=127.0.0.1:<tcp_port>``.

The test binary is started twice:

* **Server role**: runs the ``l2cap_server::...`` ztest suite. It registers an
  L2CAP BR/EDR server using ``bt_l2cap_br_server_register()`` and publishes a
  dynamic PSM through an SDP record (Protocol Descriptor List).

* **Client role**: runs the ``l2cap_client::...`` ztest suite. It performs GAP
  inquiry to find the peer, establishes a BR/EDR ACL connection, uses SDP
  discovery to retrieve the server PSM, and then opens an L2CAP BR/EDR channel
  with the requested mode.

Test Scenarios
**************

Scenarios are defined in :file:`tests.yaml` and implemented by scripts in
:file:`tests_scripts/`.

The scripts follow the common Bluetooth Classic simulation harness pattern:

* allocate two free TCP ports (HCI transport),
* start two Bumble controllers, each bound to one TCP port and one fixed BD_ADDR,
* run the same ``zephyr.exe`` twice, once as the L2CAP server and once as the
  L2CAP client.

The L2CAP mode coverage in this test suite includes:

* Basic mode
* Retransmission (RET) mode
* Flow Control (FC) mode
* Enhanced Retransmission (ERET) mode
* Streaming mode

Optional mode negotiation is also covered (when the peer does not support the
requested mode and the requester allows fallback).

Available scenarios:

* ``bluetooth.classic.sim.l2cap.basic``

  * Script: :file:`tests_scripts/basic.sh`
  * Server ztest: ``-test=l2cap_server::test_01_basic_mode``
  * Client ztest: ``-test=l2cap_client::test_01_basic_mode``

* ``bluetooth.classic.sim.l2cap.basic.mode_optional``

  * Script: :file:`tests_scripts/basic_mode_optional.sh`
  * Server ztest: ``-test=l2cap_server::test_02_basic_mode_optional``
  * Client ztest: ``-test=l2cap_client::test_02_basic_mode_optional``

* ``bluetooth.classic.sim.l2cap.retransmission``

  * Script: :file:`tests_scripts/ret.sh`
  * Server ztest: ``-test=l2cap_server::test_03_ret_mode``
  * Client ztest: ``-test=l2cap_client::test_03_ret_mode``

* ``bluetooth.classic.sim.l2cap.retransmission.mode_optional``

  * Script: :file:`tests_scripts/ret_mode_optional.sh`
  * Server ztest: ``-test=l2cap_server::test_04_ret_mode_optional``
  * Client ztest: ``-test=l2cap_client::test_04_ret_mode_optional``

* ``bluetooth.classic.sim.l2cap.flow_control``

  * Script: :file:`tests_scripts/fc.sh`
  * Server ztest: ``-test=l2cap_server::test_05_fc_mode``
  * Client ztest: ``-test=l2cap_client::test_05_fc_mode``

* ``bluetooth.classic.sim.l2cap.flow_control.mode_optional``

  * Script: :file:`tests_scripts/fc_mode_optional.sh`
  * Server ztest: ``-test=l2cap_server::test_06_fc_mode_optional``
  * Client ztest: ``-test=l2cap_client::test_06_fc_mode_optional``

* ``bluetooth.classic.sim.l2cap.enhanced_retransmission``

  * Script: :file:`tests_scripts/eret.sh`
  * Server ztest: ``-test=l2cap_server::test_07_eret_mode``
  * Client ztest: ``-test=l2cap_client::test_07_eret_mode``

* ``bluetooth.classic.sim.l2cap.enhanced_retransmission.mode_optional``

  * Script: :file:`tests_scripts/eret_mode_optional.sh`
  * Server ztest: ``-test=l2cap_server::test_08_eret_mode_optional``
  * Client ztest: ``-test=l2cap_client::test_08_eret_mode_optional``

* ``bluetooth.classic.sim.l2cap.streaming``

  * Script: :file:`tests_scripts/stream.sh`
  * Server ztest: ``-test=l2cap_server::test_09_stream_mode``
  * Client ztest: ``-test=l2cap_client::test_09_stream_mode``

* ``bluetooth.classic.sim.l2cap.streaming.mode_optional``

  * Script: :file:`tests_scripts/stream_mode_optional.sh`
  * Server ztest: ``-test=l2cap_server::test_10_stream_mode_optional``
  * Client ztest: ``-test=l2cap_client::test_10_stream_mode_optional``

Requirements
************

* Host environment capable of running ``native_sim`` tests
* Bumble available for the Bluetooth Classic controller simulation

Building and Running
********************

This test is intended to be run via Twister on ``native_sim``.

Build and run with Twister
==========================

.. code-block:: console

   west twister -T tests/bluetooth/classic/sim/l2cap

Direct script execution
=======================

Twister uses the script harness defined in :file:`tests.yaml` and runs the
scripts listed in ``harness_config/tests_scripts``.

For example:

.. code-block:: console

   ./tests/bluetooth/classic/sim/l2cap/tests_scripts/basic.sh
   ./tests/bluetooth/classic/sim/l2cap/tests_scripts/ret.sh
   ./tests/bluetooth/classic/sim/l2cap/tests_scripts/fc.sh
   ./tests/bluetooth/classic/sim/l2cap/tests_scripts/eret.sh
   ./tests/bluetooth/classic/sim/l2cap/tests_scripts/stream.sh

Notes
*****

* The client discovers the peer device via BR inquiry and uses SDP to retrieve the
  server PSM (Protocol Descriptor List attribute). See client implementation in
  :file:`src/test_client.c`.

* The server registers an SDP record which exposes the dynamically allocated PSM.
  See server implementation in :file:`src/test_server.c`.

* Some modes and negotiation paths require ``CONFIG_BT_L2CAP_RET_FC`` to be
  enabled; when it is disabled, mode-specific verification is compiled out.

Test Coverage
*************

This test covers:

* Bluetooth Classic initialization on ``native_sim``
* BR/EDR inquiry and ACL connection establishment
* SDP-based discovery of an L2CAP service/PSM
* L2CAP BR/EDR channel connection/disconnection
* L2CAP data send/receive for different BR/EDR link modes (Basic/RET/FC/ERET/Streaming)
* Optional mode negotiation / fallback behavior

Configuration Options
*********************

See :file:`prj.conf` for the default configuration.

Additional configuration options can be found in :file:`Kconfig`.
