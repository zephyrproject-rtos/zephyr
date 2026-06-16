.. _bluetooth_classic_gap_discovery_test:

Bluetooth Classic GAP Discovery Test
####################################

Overview
********

This test verifies the Bluetooth Classic GAP (Generic Access Profile) discovery
functionality using the Bumble Bluetooth controller simulator sidecar.
It validates that a central and a peripheral can perform inquiry-based discovery
(via general and limited discovery procedures).

The test is executed on the Zephyr ``native_sim`` platform and uses the
`Bumble <https://github.com/google/bumble>`_ Bluetooth controller simulator.
Two simulated controllers are started by Twister's Bumble sidecar and connected
to two instances of the same test binary via ``--bt-dev=127.0.0.1:<tcp_port>``.
Each instance receives a ``--peer_bd_address`` argument so the two roles can
locate each other.

Test Cases
**********

``bluetooth.classic.sim.gap.general_discovery``
  - **Central** (``gap_central::test_01_general_discovery``): performs a general
    inquiry (``limited=false``) and verifies the peer device is found, then
    creates and disconnects an ACL connection.
  - **Peripheral** (``gap_peripheral::test_01_general_discovery``): sets itself
    connectable and generally discoverable, waits for the central to connect and
    disconnect.

``bluetooth.classic.sim.gap.limited_discovery``
  - **Central** (``gap_central::test_02_limited_discovery``): performs a limited
    inquiry (``limited=true``), verifies the peer is found and that its Class of
    Device (CoD) has the ``Limited Discoverable`` service class bit set.  After
    ``CONFIG_BT_LIMITED_DISCOVERABLE_DURATION`` elapses it runs a second inquiry
    and confirms the limited bit is no longer set.  Finally it connects and
    disconnects.
  - **Peripheral** (``gap_peripheral::test_02_limited_discovery``): sets itself
    connectable and limitedly discoverable, waits for the central to connect and
    disconnect.

Requirements
************

* Host environment capable of running ``native_sim`` tests.
* Python package ``bumble`` installed (used by Twister's Bumble sidecar).

Building and Running
********************

This test is intended to be run via Twister on ``native_sim``.

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/gap_discovery

Twister invokes the ``bumble`` sidecar defined in :file:`tests.yaml`.  The
sidecar starts two Bumble controllers with fixed BD_ADDR values
(``00:00:01:00:00:01`` and ``00:00:01:00:00:02``) and passes the matching
``--peer_bd_address`` and ``-test`` arguments to each binary instance
automatically.

If you see "west: command not found", activate the Zephyr virtual environment:

.. code-block:: console

   source ~/zephyrproject/.venv/bin/activate

Refer to :ref:`getting_started` for details.

Sample Output
*************

.. code-block:: console

   INFO    - Adding tasks to the queue...
   INFO    - Added initial list of jobs to queue
   INFO    - Total complete:    2/   2  100%  built (not run):    0, filtered:    0, failed:    0, error:    0
   INFO    - 2 test scenarios (2 configurations) selected, 0 configurations filtered (0 by static filter, 0 at runtime).
   INFO    - 2 of 2 executed test configurations passed (100.00%), 0 built (not run), 0 failed, 0 errored, with no warnings in 115.33 seconds.
   INFO    - 2 of 2 executed test cases passed (100.00%) on 1 out of total 1560 platforms (0.06%).
   INFO    - 2 test configurations executed on platforms, 0 test configurations were only built.
   INFO    - Saving reports...

Test Coverage
*************

This test covers:

* Bluetooth Classic initialization and de-initialization
* GAP general inquiry start/stop (``bt_br_discovery_start`` / ``bt_br_discovery_stop``)
* GAP limited inquiry start/stop
* Discovery callbacks (``bt_br_discovery_cb`` ``recv`` and ``timeout``)
* Inquiry result handling (address matching, RSSI, CoD)
* Limited Discoverable CoD service class bit verification
* ``bt_br_set_connectable`` / ``bt_br_set_discoverable`` (general and limited)
* ACL connection creation and disconnection (``bt_conn_create_br`` /
  ``bt_conn_disconnect``)
* ``CONFIG_BT_LIMITED_DISCOVERABLE_DURATION`` expiry behaviour

Configuration Options
*********************

See :file:`prj.conf` for the default configuration.

Additional configuration options can be found in :file:`Kconfig`.
