.. _bluetooth_classic_sdp_discovery_test:

Bluetooth Classic SDP Discovery Test
####################################

Overview
********

This test suite verifies Bluetooth Classic SDP (Service Discovery Protocol)
client/server discovery behavior using the Bumble Bluetooth controller simulator
sidecar.

The test is executed on the Zephyr ``native_sim`` platform and uses the
`Bumble <https://github.com/google/bumble>`_ Bluetooth controller simulator.
Two simulated controllers are started by Twister's Bumble sidecar and connected
to two instances of the same test binary via ``--bt-dev=127.0.0.1:<tcp_port>``.
Each instance receives a ``--peer_bd_address`` argument so the two roles can
locate each other.

The test binary is started twice:

* **Server role** (``sdp_server::...`` ztest suite): sets itself connectable and
  discoverable, registers the requested SDP records, and waits for the client to
  connect and disconnect.
* **Client role** (``sdp_client::...`` ztest suite): connects to the server,
  performs the SDP queries, verifies the results, and disconnects.

Test Cases
**********

``bluetooth.classic.sim.sdp.discover.no_records``
  Server registers no SDP records.  Client queries the server and verifies an
  empty result set is returned correctly.

``bluetooth.classic.sim.sdp.discover.one_record``
  Server registers one custom SDP record (UUID128 service class, L2CAP +
  custom-protocol descriptor list, additional protocol descriptor list, profile
  descriptor list, supported features, and service name).  Client queries and
  validates the single record.

``bluetooth.classic.sim.sdp.discover.one_record_with_range``
  Same single record as above; client additionally exercises attribute range
  queries against it.

``bluetooth.classic.sim.sdp.discover.multiple_records``
  Server registers the single custom record plus
  ``CONFIG_TEST_ADDITIONAL_SDP_RECORD_COUNT`` SPP records and two extra SPP
  records with long service names (one oversized, one at the valid size limit).
  Client queries all records and verifies the results.

``bluetooth.classic.sim.sdp.discover.multiple_records_with_range``
  Same set of multiple records as above; client additionally exercises attribute
  range queries across all records.

Requirements
************

* Host environment capable of running ``native_sim`` tests.
* Python package ``bumble`` installed (used by Twister's Bumble sidecar).

Building and Running
********************

This test is intended to be run via Twister on ``native_sim``.

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/sdp_discovery

Twister invokes the ``bumble`` sidecar defined in :file:`tests.yaml`.  The
sidecar starts two Bumble controllers with fixed BD_ADDR values
(``00:00:01:00:00:01`` and ``00:00:01:00:00:02``) and passes the matching
``--peer_bd_address`` and ``-test`` arguments to each binary instance
automatically.

To run a single test scenario:

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/sdp_discovery \
       -s bluetooth.classic.sim.sdp.discover.one_record

If you see "west: command not found", activate the Zephyr virtual environment:

.. code-block:: console

   source ~/zephyrproject/.venv/bin/activate

Refer to :ref:`getting_started` for details.

Sample Output
*************

.. code-block:: console

   INFO    - Adding tasks to the queue...
   INFO    - Added initial list of jobs to queue
   INFO    - Total complete:    5/   5  100%  built (not run):    0, filtered:    0, failed:    0, error:    0
   INFO    - 5 test scenarios (5 configurations) selected, 0 configurations filtered (0 by static filter, 0 at runtime).
   INFO    - 5 of 5 executed test configurations passed (100.00%), 0 built (not run), 0 failed, 0 errored, with no warnings in XX.XX seconds.
   INFO    - Saving reports...

Test Coverage
*************

This test covers:

* Bluetooth Classic initialization and de-initialization
* ``bt_br_set_connectable`` / ``bt_br_set_discoverable``
* ACL connection establishment and disconnection
* ``bt_sdp_register_service`` with zero, one, and multiple records
* SDP service search and attribute query procedures (``sdp_client``)
* SDP attribute range queries
* SDP record with UUID128 service class, L2CAP/custom protocol descriptor
  lists, additional protocol descriptor list, profile descriptor list,
  supported features, and service name
* SDP records with SPP (RFCOMM) service class and varying RFCOMM channel numbers
* Handling of oversized vs. valid-length SDP service name strings

Configuration Options
*********************

See :file:`prj.conf` for the default configuration.

Additional configuration options, including
``CONFIG_TEST_ADDITIONAL_SDP_RECORD_COUNT``, can be found in :file:`Kconfig`.
