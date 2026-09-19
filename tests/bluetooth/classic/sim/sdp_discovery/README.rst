.. _bluetooth_classic_sdp_discovery_test:

Bluetooth Classic SDP Discovery Test
####################################

Overview
********

This test suite verifies Bluetooth Classic SDP (Service Discovery Protocol)
client/server discovery behavior using the Bluetooth Classic simulation test
infrastructure.

The test is executed on the Zephyr ``native_sim`` platform and uses the
`Bumble <https://github.com/google/bumble>`_ Bluetooth controller simulator.
Two simulated controllers are started and connected to two instances of the same
test binary via ``--bt-dev=127.0.0.1:<tcp_port>``.

The test binary is started twice:

* **Server role**: runs the ``sdp_server::...`` ztest suite and exposes a
  specific SDP database (0/1/multiple records, optionally with attribute range
  queries).
* **Client role**: runs the ``sdp_client::...`` ztest suite and performs SDP
  discovery against the server.

Test Scenarios
**************

Scenarios are defined in :file:`tests.yaml` and implemented by scripts in
:file:`tests_scripts/`.

The scripts follow the common Bluetooth Classic simulation harness pattern:

* allocate two free TCP ports (HCI transport),
* start two Bumble controllers, each bound to one TCP port and one fixed BD_ADDR,
* run the same ``zephyr.exe`` twice, once as the SDP server and once as the SDP
  client.

Available scenarios:

* ``bluetooth.classic.sim.sdp.discover.no_records``

  * Script: :file:`tests_scripts/no_sdp_records.sh`
  * Server ztest: ``-test=sdp_server::test_01_no_sdp_records``
  * Client ztest: ``-test=sdp_client::test_01_no_sdp_records``

* ``bluetooth.classic.sim.sdp.discover.one_record``

  * Script: :file:`tests_scripts/one_sdp_record.sh`
  * Server ztest: ``-test=sdp_server::test_02_one_sdp_record``
  * Client ztest: ``-test=sdp_client::test_02_one_sdp_record``

* ``bluetooth.classic.sim.sdp.discover.one_record_with_range``

  * Script: :file:`tests_scripts/one_sdp_record_with_range.sh`
  * Server ztest: ``-test=sdp_server::test_03_one_sdp_record_with_range``
  * Client ztest: ``-test=sdp_client::test_03_one_sdp_record_with_range``

* ``bluetooth.classic.sim.sdp.discover.multiple_records``

  * Script: :file:`tests_scripts/multiple_sdp_records.sh`
  * Server ztest: ``-test=sdp_server::test_04_multiple_sdp_records``
  * Client ztest: ``-test=sdp_client::test_04_multiple_sdp_records``

* ``bluetooth.classic.sim.sdp.discover.multiple_records_with_range``

  * Script: :file:`tests_scripts/multiple_sdp_records_with_range.sh`
  * Server ztest: ``-test=sdp_server::test_05_multiple_sdp_records_with_range``
  * Client ztest: ``-test=sdp_client::test_05_multiple_sdp_records_with_range``

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

   west twister -T tests/bluetooth/classic/sim/sdp_discovery

Direct script execution
=======================

Twister uses the script harness defined in :file:`tests.yaml` and runs the
scripts listed in ``harness_config/tests_scripts``.

The following scripts implement the scenarios listed above:

.. code-block:: console

   ./tests/bluetooth/classic/sim/sdp_discovery/tests_scripts/no_sdp_records.sh
   ./tests/bluetooth/classic/sim/sdp_discovery/tests_scripts/one_sdp_record.sh
   ./tests/bluetooth/classic/sim/sdp_discovery/tests_scripts/one_sdp_record_with_range.sh
   ./tests/bluetooth/classic/sim/sdp_discovery/tests_scripts/multiple_sdp_records.sh
   ./tests/bluetooth/classic/sim/sdp_discovery/tests_scripts/multiple_sdp_records_with_range.sh

Each script:

* allocates two free TCP ports (HCI over TCP),
* starts two Bumble controllers bound to those ports, using fixed BD_ADDR values,
* locates the Twister-built test binary via ``extra_twister_out`` (from
  :file:`../common/common_utils.sh`),
* starts the same test binary twice:

  * SDP server instance, passing ``--peer_bd_address=<peer>`` and
    ``-test=sdp_server::...``
  * SDP client instance, passing ``--peer_bd_address=<peer>`` and
    ``-test=sdp_client::...``

Notes
-----

* The scripts rely on environment variables set by Twister's script harness
  (for example ``ZEPHYR_BASE`` and ``BOARD``; ``BOARD_SANITIZED`` is derived in
  :file:`../common/common_utils.sh`).
* If running outside of Twister, ensure ``west`` is available and the Zephyr
  environment is activated. Refer to :ref:`getting_started` for details.

.. code-block:: console

   source ~/zephyrproject/.venv/bin/activate

Sample Output
*************

.. code-block:: console

   INFO    - Adding tasks to the queue...
   INFO    - Added initial list of jobs to queue
   INFO    - Total complete:    1/   1  100%  built (not run):    0, filtered:    0, failed:    0, error:    0
   INFO    - 1 test scenarios (1 configurations) selected, 0 configurations filtered (0 by static filter, 0 at runtime).
   INFO    - 1 of 1 executed test configurations passed (100.00%), 0 built (not run), 0 failed, 0 errored, with no warnings in XX.XX seconds.
   INFO    - Saving reports...

Test Coverage
*************

This test covers:

* Bluetooth Classic initialization
* BR/EDR connection establishment used for SDP procedures
* SDP service search and attribute discovery procedures
* Handling of empty and multiple SDP record sets

Configuration Options
*********************

See :file:`prj.conf` for the default configuration.

Additional configuration options can be found in :file:`Kconfig`.
