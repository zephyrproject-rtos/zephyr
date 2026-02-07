.. _bluetooth_classic_gap_discovery_test:

Bluetooth Classic GAP Discovery Test
#####################################

Overview
********

This test verifies the Bluetooth Classic GAP (Generic Access Profile) discovery
functionality. It tests the device's ability to discover nearby Bluetooth Classic
devices through inquiry procedures.

Requirements
************

* A board with Bluetooth Classic support
* Bluetooth Classic controller enabled in the configuration

Building and Running
********************

This test can be built and executed on boards with Bluetooth Classic support.

The following command build test:

.. code-block:: console

   ./tests/bluetooth/classic/gap_discovery/compile.sh

The following command run test:

.. code-block:: console

   ./tests/bluetooth/classic/gap_discovery/tests_scripts/discovery.sh

If the error "west: command not found" occurs, the following command should be used to activate the
virtual environment. Refer to :ref:`getting_started` for details.

.. code-block:: console

   source ~/zephyrproject/.venv/bin/activate

Sample Output
*************

.. code-block:: console

    ------ TESTSUITE SUMMARY START ------

    SUITE PASS - 100.00% [gap_central]: pass = 2, fail = 0, skip = 0, total = 2 duration = 46.640 seconds
     - PASS - [gap_central.test_01_gap_central_general_discovery] duration = 5.180 seconds
     - PASS - [gap_central.test_02_gap_central_limited_discovery] duration = 41.460 seconds

    ------ TESTSUITE SUMMARY END ------

    ------ TESTSUITE SUMMARY START ------

    SUITE PASS - 100.00% [gap_peripheral]: pass = 2, fail = 0, skip = 0, total = 2 duration = 48.620 seconds
     - PASS - [gap_peripheral.test_01_gap_peripheral_general_discovery] duration = 7.180 seconds
     - PASS - [gap_peripheral.test_02_gap_peripheral_limited_discovery] duration = 41.440 seconds

    ------ TESTSUITE SUMMARY END ------

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
