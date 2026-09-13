.. _bluetooth_classic_gap_discovery_test:

Bluetooth Classic GAP Discovery Test
####################################

Overview
********

This test verifies the Bluetooth Classic GAP (Generic Access Profile) discovery
functionality using the Bluetooth Classic simulation test infrastructure.
It validates that a central and a peripheral can perform inquiry-based discovery
(via general and limited discovery procedures).

The test is executed on the Zephyr ``native_sim`` platform and uses the
`Bumble <https://github.com/google/bumble>`_ Bluetooth controller simulator.
Two simulated controllers are started and connected to two instances of the same
test binary via ``--bt-dev=127.0.0.1:<tcp_port>``.

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

   west twister -T tests/bluetooth/classic/sim/gap_discovery

Direct script execution
=======================

Twister uses the script harness defined in :file:`tests.yaml` and runs:
:file:`tests_scripts/general_discovery.sh` or :file:`tests_scripts/limited_discovery.sh`.

.. code-block:: console

   ./tests/bluetooth/classic/sim/gap_discovery/tests_scripts/general_discovery.sh
   ./tests/bluetooth/classic/sim/gap_discovery/tests_scripts/limited_discovery.sh

The script:

* allocates two TCP ports,
* starts two Bumble controllers (one per port, with fixed BD_ADDR values),
* launches the test binary twice:

  * ``-test=gap_peripheral::*`` with ``--peer_bd_address=${BD_ADDR[1]}``
  * ``-test=gap_central::*`` with ``--peer_bd_address=${BD_ADDR[0]}``

If you see "west: command not found", activate the Zephyr virtual environment.
Refer to :ref:`getting_started` for details.

.. code-block:: console

   source ~/zephyrproject/.venv/bin/activate

Sample Output
*************

.. code-block:: console

INFO    - Adding tasks to the queue...
INFO    - Added initial list of jobs to queue
INFO    - Total complete:    1/   1  100%  built (not run):    0, filtered:    0, failed:    0, error:    0
INFO    - 1 test scenarios (1 configurations) selected, 0 configurations filtered (0 by static filter, 0 at runtime).
INFO    - 1 of 1 executed test configurations passed (100.00%), 0 built (not run), 0 failed, 0 errored, with no warnings in 115.33 seconds.
INFO    - 1 of 1 executed test cases passed (100.00%) on 1 out of total 1560 platforms (0.06%).
INFO    - 1 test configurations executed on platforms, 0 test configurations were only built.
INFO    - Saving reports...

Test Coverage
*************

This test covers:

* Bluetooth Classic initialization
* GAP inquiry start/stop procedures
* Device discovery callbacks
* Inquiry result handling

Configuration Options
*********************

See :file:`prj.conf` for the default configuration.

Additional configuration options can be found in :file:`Kconfig`.
