.. _bluetooth_classic_sim:

Bluetooth Classic Simulation Testing
#####################################

Overview
********

The Bluetooth Classic simulation testing framework provides a way to test Bluetooth Classic
(BR/EDR) functionality in Zephyr without requiring physical hardware. This framework uses
Python-based simulated controllers based on the Bumble Bluetooth stack (version 0.0.220)
and allows multiple simulated devices to interact over TCP connections.

The end-to-end test setup consists of:

* A Bumble-based controller simulator
  (:zephyr_file:`tests/bluetooth/classic/bumble/common/controllers.py`)
* Twister's ``bumble`` sidecar
  (:zephyr_file:`scripts/pylib/twister/twisterlib/sidecars/bumble.py`), which starts the
  controllers and the peer executables automatically
* One or more :zephyr:board:`native_sim` test executables launched by Twister's ``ztest`` harness

Architecture
************

Simulated Controllers
=====================

The framework uses Python-based simulated Bluetooth Classic controllers built on
**Bumble 0.0.220**. These controllers implement the HCI interface over TCP and provide:

* **BR/EDR Support**: Full Bluetooth Classic (BR/EDR) functionality
* **HCI Interface**: Standard HCI communication over TCP
* **Multiple Controllers**: Support for running multiple simulated devices simultaneously
* **Link Simulation**: Simulated radio links between controllers for device discovery and
  connections
* **Bumble-based**: Leverages the Bumble Bluetooth stack (version 0.0.220) for protocol
  implementation

The controller implementation can be found in:

* :zephyr_file:`tests/bluetooth/classic/bumble/common/controllers.py` — Main controller entry point

Supporting simulation building blocks used by the controller:

* :zephyr_file:`tests/bluetooth/classic/bumble/common/bt_sim_controller.py`
* :zephyr_file:`tests/bluetooth/classic/bumble/common/bt_sim_hci.py`
* :zephyr_file:`tests/bluetooth/classic/bumble/common/bt_sim_link.py`
* :zephyr_file:`tests/bluetooth/classic/bumble/common/bt_sim_ll.py`

Bumble Sidecar
==============

The Twister ``bumble`` sidecar (:zephyr_file:`scripts/pylib/twister/twisterlib/sidecars/bumble.py`)
automates the lifecycle of controllers and peer executables. For each test run it:

1. Allocates one free TCP port per controller using ``socket.bind(('127.0.0.1', 0))``.
2. Launches ``controllers.py`` with one ``tcp-server:_:<port>@<bd_address>`` argument per
   controller so all controllers share a single process and a simulated radio link.
3. Waits until all TCP ports accept connections (up to 5 seconds).
4. Injects ``--bt-dev=127.0.0.1:<port>`` and any per-device arguments into the primary Zephyr
   instance that the normal ``ztest`` harness runs (device 0).
5. Spawns each additional peer (devices 1..N-1) as independent subprocesses against their own
   controller ports.
6. In ``teardown``, collects the peer exit codes and fails the test if any peer exits non-zero,
   then terminates the controllers.

Test Applications
=================

Test applications are standard Zephyr applications built for the :zephyr:board:`native_sim` board.
Each application can run different test suites based on runtime parameters passed through
``sidecar_config``. Tests use the Ztest framework for test case definition and execution.

Port and BD Address Mapping
============================

The ``bumble`` sidecar establishes a clear mapping between TCP ports and Bluetooth device addresses
to enable predictable test scenarios and device role assignment.

Configured Addresses
--------------------

Addresses are specified per-test in the ``addresses`` list inside ``sidecar_config.bumble`` in
:file:`tests.yaml`. The default when no ``addresses`` key is present is:

.. code-block:: yaml

   addresses:
     - 00:00:01:00:00:01
     - 00:00:01:00:00:02

These follow a sequential pattern starting from ``00:00:01:00:00:01``.

Port Allocation
---------------

TCP ports are allocated dynamically by the sidecar.  Each test instance uses unique ports,
enabling parallel test execution without conflicts.

Port-to-Address Binding
------------------------

When launching ``controllers.py`` the sidecar passes each controller as
``tcp-server:_:<port>@<bd_address>``, which creates a one-to-one mapping between the HCI transport
port and the controller's Bluetooth address.

Role-to-Address Mapping in Tests
---------------------------------

Test cases establish a mapping between device roles and BD addresses by:

1. **Passing peer addresses as runtime parameters**: Test executables accept a
   ``--peer_bd_address=XX:XX:XX:XX:XX:XX`` parameter to specify the expected peer device address.

   In the Bluetooth Classic simulation tests running on :zephyr:board:`native_sim`, the parameter
   is parsed using the native_sim command line options framework (see
   :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/src/test_main.c`).

2. **Matching controller creation order**: The order of addresses in
   ``sidecar_config.bumble.addresses`` determines which executable connects to which controller.

3. **Connecting executables to ports**: Each entry in ``sidecar_config.bumble.devices`` is a string
   of extra arguments for the corresponding executable; the sidecar appends ``--bt-dev``
   automatically.

**Example: GAP Discovery Test**

In :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/tests.yaml`:

.. code-block:: yaml

   tests:
     bluetooth.classic.sim.gap.general_discovery:
       sidecar_config:
         bumble:
           addresses:
             - 00:00:01:00:00:01
             - 00:00:01:00:00:02
           devices:
             - --peer_bd_address=00:00:01:00:00:02 -test=gap_central::test_01_general_discovery
             - --peer_bd_address=00:00:01:00:00:01 -test=gap_peripheral::test_01_general_discovery

In this example:

- Device 0 (the primary guest, run by the ``ztest`` harness) connects to controller 0, whose BD
  address is ``00:00:01:00:00:01``.  It is given the ``gap_central`` role and is told its peer's
  address is ``00:00:01:00:00:02``.
- Device 1 (the sidecar peer) connects to controller 1, whose BD address is ``00:00:01:00:00:02``.
  It is given the ``gap_peripheral`` role and is told its peer's address is ``00:00:01:00:00:01``.

This mapping allows test applications to:

- Know the BD addresses of peer devices in advance
- Configure expected peer addresses via runtime parameters
- Verify discovery results against known addresses
- Establish connections to specific peer devices
- Run different test suites from the same executable

Test execution (Twister + bumble sidecar)
=========================================

The Bluetooth Classic simulation tests are standard Twister tests that use ``harness: ztest`` and
``sidecar: bumble``.

* Each test directory provides its own :file:`tests.yaml` (for example
  :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/tests.yaml`).
* Twister builds the ``native_sim`` executable as usual and then calls the ``bumble`` sidecar
  before and after the test run.
* The sidecar starts the linked Bumble controllers and any peer executables.
* The primary executable is launched by the ``ztest`` harness; its output is parsed for pass/fail
  as normal.

Run the whole suite:

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble

Run a single test directory:

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/gap_discovery

Run a single scenario:

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/gap_discovery \
       -s bluetooth.classic.sim.gap.general_discovery

Directory Structure
*******************

.. code-block:: none

   tests/bluetooth/classic/bumble/
   ├── common/                             # Shared controller implementation
   │   ├── controllers.py                  # Controller entry point (Bumble-based)
   │   ├── bt_sim_controller.py            # Controller building block
   │   ├── bt_sim_hci.py                   # HCI transport building block
   │   ├── bt_sim_link.py                  # Link simulation building block
   │   └── bt_sim_ll.py                    # Link-layer building block
   ├── gap_discovery/                      # GAP inquiry and discovery test
   │   ├── src/
   │   │   ├── test_main.c                 # native_sim cmdline parsing
   │   │   ├── test_central.c              # Central role tests
   │   │   └── test_peripheral.c           # Peripheral role tests
       ├── CMakeLists.txt                  # CMake build configuration
       ├── Kconfig                         # Kconfig options
       ├── prj.conf                        # Project configuration
       ├── README.rst                      # Test documentation
       └── tests.yaml                      # Test case definitions
   ├── sdp_discovery/                      # SDP client/server test
   ├── l2cap/                              # L2CAP BR/EDR channel test
   ├── l2cap_echo/                         # L2CAP Echo Request/Response test
   └── l2cap_connectless/                  # L2CAP Connectionless channel test

Building and Running Tests
***************************

Prerequisites
=============

1. **Zephyr Environment**: Source the Zephyr environment:

   .. code-block:: bash

      source zephyr/zephyr-env.sh

2. **Python Dependencies**: Install Bumble 0.0.220:

   .. code-block:: bash

      pip install bumble==0.0.220

3. **Build Tools**: Ensure you have the standard Zephyr build tools (cmake, ninja, etc.)

Building Tests
==============

Build via Twister (recommended):

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble --build-only

Build a specific test:

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/gap_discovery --build-only

Running Tests
=============

Run via Twister:

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble

Writing New Tests
*****************

Test Structure
==============

Each test directory should contain:

.. code-block:: none

   my_test/
   ├── src/                       # Test source code
   │   ├── test_main.c            # Main entry point with parameter parsing
   │   ├── test_role1.c           # Role 1 test implementation (Ztest)
   │   └── test_role2.c           # Role 2 test implementation (Ztest)
   ├── CMakeLists.txt             # CMake build configuration
   ├── Kconfig                    # Kconfig options (optional)
   ├── prj.conf                   # Project configuration
   ├── README.rst                 # Test documentation
   └── tests.yaml                 # Test case definitions

Tests use separate source files for different device roles, with each role implementing its test
logic using the Ztest framework. A main entry point (``test_main.c``) handles parameter parsing
and test suite selection.

Test Case Definition (tests.yaml)
====================================

The :file:`tests.yaml` file defines the test scenarios using the ``bumble`` sidecar:

.. code-block:: yaml

   common:
     tags:
       - bluetooth
     platform_allow:
       - native_sim
     harness: ztest
     sidecar: bumble

   tests:
     bluetooth.classic.my_test.scenario1:
       timeout: 120
       sidecar_config:
         bumble:
           addresses:
             - 00:00:01:00:00:01
             - 00:00:01:00:00:02
           devices:
             - --peer_bd_address=00:00:01:00:00:02 -test=role1_suite::test_01
             - --peer_bd_address=00:00:01:00:00:01 -test=role2_suite::test_01

The ``devices`` list maps index-for-index onto the ``addresses`` list:

* ``devices[0]`` — extra arguments injected into the primary guest (device 0, run by the ``ztest``
  harness against ``addresses[0]``).
* ``devices[1]`` — extra arguments for the first sidecar peer (run against ``addresses[1]``).

``{addrN}`` and ``{ctrlN}`` placeholders in device argument strings are expanded by the sidecar
to the BD address and ``ip:port`` of controller N.

CMakeLists.txt Configuration
=============================

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.20.0)
   find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
   project(bt_classic_my_test)

   target_sources(app PRIVATE
     src/test_main.c
     src/test_role1.c
     src/test_role2.c
   )

Test Application Code
=====================

Test applications use the Ztest framework for test case definition and execution.

For :zephyr:board:`native_sim` based tests, command line arguments are registered using the
native_sim command line option framework (``cmdline.h``).
For example, the GAP discovery test registers a mandatory ``peer_bd_address`` string option and
parses it with ``bt_addr_from_str()``:

- :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/src/test_main.c`

.. code-block:: c

   #include "cmdline.h" /* native_sim command line options header */

   extern bt_addr_t peer_addr;

   static void cmd_peer_bd_address_found(char *argv, int offset)
   {
       char *addr_str = &argv[offset];
       int err;

       err = bt_addr_from_str(addr_str, &peer_addr);
       if (err != 0) {
           LOG_ERR("Failed to parse peer Bluetooth address: %s (err %d)", addr_str, err);
           nsi_exit(err);
       }
   }

   static void gap_discovery_args(void)
   {
       static struct args_struct_t args[] = {
           {
               .is_mandatory = true,
               .option = "peer_bd_address",
               .name = "XX:XX:XX:XX:XX:XX",
               .type = 's',
               .call_when_found = cmd_peer_bd_address_found,
               .descript = "Bluetooth peer device address for GAP discovery test",
           },
           ARG_TABLE_ENDMARKER
       };

       native_add_command_line_opts(args);
   }

   NATIVE_TASK(gap_discovery_args, PRE_BOOT_1, 20);

**Central Role Example** (``test_central.c``):

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <zephyr/bluetooth/bluetooth.h>
   #include <zephyr/bluetooth/hci.h>
   #include <zephyr/logging/log.h>
   #include <zephyr/ztest.h>

   extern bt_addr_t peer_addr;

   static K_SEM_DEFINE(discovery_sem, 0, 1);

   static void br_discover_recv(const struct bt_br_discovery_result *result)
   {
       if (bt_addr_eq(&peer_addr, &result->addr)) {
           k_sem_give(&discovery_sem);
       }
   }

   static struct bt_br_discovery_cb br_discover = {
       .recv = br_discover_recv,
   };

   ZTEST(gap_central, test_01_general_discovery)
   {
       struct bt_br_discovery_param param = { .length = 10, .limited = false };
       struct bt_br_discovery_result results[10];
       int err;

       bt_br_discovery_cb_register(&br_discover);

       err = bt_br_discovery_start(&param, results, ARRAY_SIZE(results));
       zassert_equal(err, 0, "Discovery start failed (err %d)", err);

       err = k_sem_take(&discovery_sem, K_SECONDS(30));
       zassert_equal(err, 0, "Discovery timeout (err %d)", err);

       bt_br_discovery_stop();
   }

   static void *setup(void)
   {
       int err = bt_enable(NULL);

       zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);
       return NULL;
   }

   static void teardown(void *f)
   {
       bt_disable();
   }

   ZTEST_SUITE(gap_central, NULL, setup, NULL, NULL, teardown);

Key Points for Test Applications
=================================

* **Ztest Framework**: Use ``ZTEST()`` macro to define test cases.
* **Test Suites**: Use ``ZTEST_SUITE()`` to define test suites with setup/teardown.
* **Test Suite Selection**: Use ``-test`` parameter in ``sidecar_config`` to select which suite to
  run at runtime.
* **Parameter Parsing**: Implement parameter parsing in ``test_main.c`` for runtime configuration.
* **Peer Address**: Use ``--peer_bd_address`` parameter to pass peer device addresses.
* **Assertions**: Use ``zassert_*()`` macros for test assertions.
* **Synchronization**: Use semaphores or other synchronization primitives for inter-device
  coordination.
* **Logging**: Use the logging subsystem for debug output.
* **Timeouts**: Always use appropriate timeouts to prevent hanging tests.
* **Cleanup**: Ensure proper cleanup in teardown functions.

Best Practices
==============

When writing new tests:

1. **Use sequential BD addresses**: Assign ``00:00:01:00:00:01`` to device 0, ``00:00:01:00:00:02``
   to device 1, etc.

2. **Document role-to-address mapping**: Clearly document which role uses which BD address in test
   documentation and comments.

3. **Pass peer addresses as parameters**: Use ``--peer_bd_address`` in the ``devices`` list to
   inform each device about its peer's address at runtime.

4. **Use test suite selection**: Use ``-test`` to specify which test suite to run from a single
   executable.

5. **Set appropriate timeouts**: Set the ``timeout`` field in :file:`tests.yaml` to a value that
   accounts for controller startup and all test phases.

Example: GAP Discovery Test
****************************

The GAP discovery test demonstrates BR/EDR device discovery functionality.

Test Scenario
=============

1. **General Discovery**:

   * Peripheral enables general discoverable and connectable mode.
   * Central performs general inquiry (``bt_br_discovery_start`` with ``limited=false``).
   * Central discovers the peripheral and verifies its BD address.
   * Devices establish and then disconnect an ACL connection.

2. **Limited Discovery**:

   * Peripheral enables limited discoverable mode.
   * Central performs limited inquiry (``limited=true``).
   * Central verifies the Limited Discoverable CoD service class bit is set.
   * After ``CONFIG_BT_LIMITED_DISCOVERABLE_DURATION`` elapses, Central re-runs limited inquiry
     and verifies the bit is cleared.
   * Devices establish and disconnect an ACL connection.

Files
=====

* :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/src/test_main.c` — Main entry point with parameter parsing
* :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/src/test_central.c` — Central role test implementation
* :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/src/test_peripheral.c` — Peripheral role test implementation
* :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/tests.yaml` — Test case definitions
* :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/Kconfig` — Kconfig options
* :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/README.rst` — Detailed test documentation

Troubleshooting
***************

Controller Startup Failures
============================

If simulated controllers fail to start:

* Verify the Bumble package is installed: ``pip show bumble``
* Review controller logs in ``<build_dir>/bumble-controllers.log``
* Verify TCP ports are accessible
* Check that ``controllers.py`` exists at
  :zephyr_file:`tests/bluetooth/classic/bumble/common/controllers.py`

If the sidecar skips the test with
"Bumble controllers.py not found (Bluetooth Classic simulation framework present?)", the
:file:`tests/bluetooth/classic/bumble/common/` directory is missing from the workspace.

Test Timeouts
=============

If tests hang or time out:

* Increase the ``timeout`` field in :file:`tests.yaml`.
* Check application logs (``<build_dir>/bumble-peer*.log``) for blocking operations.
* Verify proper synchronization between devices.
* Ensure cleanup handlers can execute (avoid infinite loops).
* Verify the correct test suites are selected via the ``-test`` parameter in ``sidecar_config``.

Bumble Version Mismatch
=======================

If you encounter compatibility issues:

* Ensure you have exactly Bumble 0.0.220 installed: ``pip install bumble==0.0.220``
* Newer or older versions may have incompatible APIs

Peer Exit Failures
==================

If the test fails with "Bumble peer N exited with <code>":

* Open ``<build_dir>/bumble-peer<N>.log`` to see the peer's output.
* Check that the peer's ztest suite completed all test cases.
* Verify that ``--peer_bd_address`` is set correctly in ``sidecar_config``.

Ztest Assertion Failures
=========================

If Ztest assertions fail:

* Review test logs for specific assertion failures.
* Check timing between central and peripheral operations.
* Verify Bluetooth addresses are correctly passed via ``--peer_bd_address``.
* Ensure devices are in the correct state before operations.
* Verify the correct test suite is running (check ``-test`` parameter in ``sidecar_config``).

Parameter Parsing Issues
=========================

If parameters are not being recognized:

* Verify parameter format: ``--peer_bd_address=XX:XX:XX:XX:XX:XX``
* Check that the test registers the option using the native_sim cmdline framework (see
  :zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery/src/test_main.c`).
* Review logs to confirm parameters are being received.
