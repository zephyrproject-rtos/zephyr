.. _bluetooth_classic_sim_l2cap_test:

Bluetooth Classic L2CAP Test
############################

Overview
********

This test suite verifies Bluetooth Classic BR/EDR L2CAP connection establishment
and data path behavior using the Bumble Bluetooth controller simulator sidecar.

The test is executed on the Zephyr ``native_sim`` platform and uses the
`Bumble <https://github.com/google/bumble>`_ Bluetooth controller simulator.
Two simulated controllers are started by Twister's Bumble sidecar and connected
to two instances of the same test binary via ``--bt-dev=127.0.0.1:<tcp_port>``.
Each instance receives a ``--peer_bd_address`` argument so the two roles can
locate each other.

The test binary is started twice:

* **Server role** (``l2cap_server::...`` ztest suite): sets itself connectable
  and discoverable, registers an L2CAP BR/EDR server using
  ``bt_l2cap_br_server_register()`` with a dynamically allocated PSM, and
  publishes that PSM through an SDP record.

* **Client role** (``l2cap_client::...`` ztest suite): performs GAP inquiry to
  find the peer, establishes a BR/EDR ACL connection, uses SDP discovery to
  retrieve the server PSM, and then opens an L2CAP BR/EDR channel with the
  requested mode.

Test Cases
**********

Each mode has two scenarios: a strict scenario where both sides must use the
requested mode, and an ``mode_optional`` scenario where the requester allows
fallback if the peer does not support the requested mode.

``bluetooth.classic.sim.l2cap.basic``
  Both sides negotiate Basic mode.
  Extra conf: :file:`prj.conf` (default).

``bluetooth.classic.sim.l2cap.basic.mode_optional``
  Client requests Basic mode with optional fallback.
  Extra conf: :file:`prj_basic.conf`.

``bluetooth.classic.sim.l2cap.retransmission``
  Both sides negotiate Retransmission (RET) mode.
  Extra conf: :file:`prj_ret.conf`.

``bluetooth.classic.sim.l2cap.retransmission.mode_optional``
  Client requests RET mode with optional fallback.
  Extra conf: :file:`prj_ret.conf`.

``bluetooth.classic.sim.l2cap.flow_control``
  Both sides negotiate Flow Control (FC) mode.
  Extra conf: :file:`prj_fc.conf`.

``bluetooth.classic.sim.l2cap.flow_control.mode_optional``
  Client requests FC mode with optional fallback.
  Extra conf: :file:`prj_fc.conf`.

``bluetooth.classic.sim.l2cap.enhanced_retransmission``
  Both sides negotiate Enhanced Retransmission (ERET) mode.
  Extra conf: :file:`prj_eret.conf`.

``bluetooth.classic.sim.l2cap.enhanced_retransmission.mode_optional``
  Client requests ERET mode with optional fallback.
  Extra conf: :file:`prj_eret.conf`.

``bluetooth.classic.sim.l2cap.streaming``
  Both sides negotiate Streaming mode.
  Extra conf: :file:`prj_stream.conf`.

``bluetooth.classic.sim.l2cap.streaming.mode_optional``
  Client requests Streaming mode with optional fallback.
  Extra conf: :file:`prj_stream.conf`.

``bluetooth.classic.sim.l2cap.multiple_channels_simultaneously.basic``
  Both sides negotiate Basic mode.
  Extra conf: :file:`prj_basic.conf`.

``bluetooth.classic.sim.l2cap.multiple_channels_simultaneously.eret``
  Both sides negotiate Basic mode.
  Extra conf: :file:`prj_eret.conf`.

Requirements
************

* Host environment capable of running ``native_sim`` tests.
* Python package ``bumble`` installed (used by Twister's Bumble sidecar).

Building and Running
********************

This test is intended to be run via Twister on ``native_sim``.

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/l2cap

Twister invokes the ``bumble`` sidecar defined in :file:`tests.yaml`. The
sidecar starts two Bumble controllers with fixed BD_ADDR values
(``00:00:01:00:00:01`` and ``00:00:01:00:00:02``) and passes the matching
``--peer_bd_address`` and ``-test`` arguments to each binary instance
automatically. Each mode-specific scenario passes the corresponding
``EXTRA_CONF_FILE`` to select the right L2CAP mode configuration.

To run a single test scenario:

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/l2cap \
       -s bluetooth.classic.sim.l2cap.basic

If you see "west: command not found", activate the Zephyr virtual environment:

.. code-block:: console

   source ~/zephyrproject/.venv/bin/activate

Refer to :ref:`getting_started` for details.

Notes
*****

* The client discovers the peer device via BR inquiry and uses SDP to retrieve
  the server PSM (Protocol Descriptor List attribute). See client
  implementation in :file:`src/test_client.c`.

* The server registers an SDP record which exposes the dynamically allocated
  PSM. See server implementation in :file:`src/test_server.c`.

* Retransmission, Flow Control, Enhanced Retransmission, and Streaming mode
  scenarios set ``CONFIG_TEST_L2CAP_SERVER_TX_DATA_POOL_COUNT=6`` and
  ``CONFIG_TEST_L2CAP_CLIENT_TX_DATA_POOL_COUNT=6`` to accommodate the larger
  TX data pools required by those modes.

* Some modes and negotiation paths require ``CONFIG_BT_L2CAP_RET_FC`` to be
  enabled; when it is disabled, mode-specific verification is compiled out.

Test Coverage
*************

This test covers:

* Bluetooth Classic initialization and de-initialization
* BR/EDR inquiry and ACL connection establishment
* SDP-based discovery of an L2CAP service/PSM
* L2CAP BR/EDR channel connection and disconnection
* L2CAP data send/receive in Basic, Retransmission, Flow Control, Enhanced
  Retransmission, and Streaming modes
* Optional mode negotiation / fallback behavior for all five modes

Configuration Options
*********************

See :file:`prj.conf` for the default configuration.

Mode-specific overlay files:

* :file:`prj_basic.conf` - Basic mode optional fallback
* :file:`prj_ret.conf` - Retransmission mode
* :file:`prj_fc.conf` - Flow Control mode
* :file:`prj_eret.conf` - Enhanced Retransmission mode
* :file:`prj_stream.conf` - Streaming mode

Additional configuration options can be found in :file:`Kconfig`.
