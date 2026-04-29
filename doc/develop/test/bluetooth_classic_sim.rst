.. _bluetooth_classic_sim:

Bluetooth Classic Simulation Testing
#####################################

Overview
********

The Bluetooth Classic simulation testing framework provides a way to test Bluetooth Classic
(BR/EDR) functionality in Zephyr without requiring physical hardware. This framework uses
simulated controllers based on the Bumble Bluetooth stack (version 0.0.220) and allows
multiple simulated devices to interact over TCP connections.

Architecture
************

The Bluetooth Classic simulation framework consists of several key components:

Simulated Controllers
=====================

The framework uses Python-based simulated Bluetooth Classic controllers built on
**Bumble 0.0.220**. These controllers implement the HCI interface and provide:

* **BR/EDR Support**: Full Bluetooth Classic (BR/EDR) functionality
* **HCI Interface**: Standard HCI communication over TCP
* **Multiple Controllers**: Support for running multiple simulated devices simultaneously
* **Link Simulation**: Simulated radio links between controllers for device discovery and connections
* **Bumble-based**: Leverages the Bumble Bluetooth stack (version 0.0.220) for protocol implementation

The controller implementation can be found in:

* :zephyr_file:`tests/bluetooth/classic/sim/common/controllers.py` - Main controller simulator based on Bumble 0.0.220

Test Applications
=================

Test applications are standard Zephyr applications built for the :zephyr:board:`native_sim` board.
Each application can run different test suites based on runtime parameters. Tests use the Ztest
framework for test case definition and execution.

Test Scripts
============

Test scripts orchestrate the execution of simulated controllers and test applications. They:

1. Allocate TCP ports for HCI communication
2. Start simulated Bluetooth controllers (Bumble-based)
3. Launch test application executables with appropriate parameters
4. Wait for test completion
5. Clean up resources (ports, processes)

Port and BD Address Mapping
============================

The framework establishes a clear mapping between TCP ports and Bluetooth device addresses
to enable predictable test scenarios and device role assignment.

Predefined BD Addresses
------------------------

The framework provides 16 predefined Bluetooth device addresses in
:zephyr_file:`tests/bluetooth/classic/sim/common/parameter_utils.sh`:

.. code-block:: bash

   BD_ADDR=(
       "00:00:01:00:00:01"
       "00:00:01:00:00:02"
       "00:00:01:00:00:03"
       # ... up to
       "00:00:01:00:00:10"
   )

These addresses follow a sequential pattern starting from ``00:00:01:00:00:01`` and are
available for use in test scenarios.

Port Allocation
----------------

Test scripts allocate TCP ports dynamically using the ``allocate_tcp_port`` function from
:zephyr_file:`tests/bluetooth/classic/sim/common/port_utils.sh`. This ensures that each
test instance uses unique ports, enabling parallel test execution without conflicts.

Port-to-Address Binding
------------------------

When creating simulated Bluetooth controllers via
:zephyr_file:`tests/bluetooth/classic/sim/common/controllers.py`, the TCP port and BD
address are passed together as a parameter in the format ``PORT@BD_ADDRESS``. This creates
a one-to-one mapping between the HCI transport port and the controller's Bluetooth address.

For example:

.. code-block:: bash

   start_bumble_controllers \
       "$BUMBLE_CONTROLLERS_LOG" \
       "$HCI_PORT_0@${BD_ADDR[0]}" \
       "$HCI_PORT_1@${BD_ADDR[1]}"

This command creates two controllers:
- Controller 1: Accessible via ``HCI_PORT_0``, with BD address ``00:00:01:00:00:01``
- Controller 2: Accessible via ``HCI_PORT_1``, with BD address ``00:00:01:00:00:02``

Role-to-Address Mapping in Tests
---------------------------------

Test cases can establish a mapping between device roles and BD addresses by:

1. **Passing peer addresses as runtime parameters**: Test executables accept a
   ``--peer_bd_address`` parameter to specify the expected peer device address.

2. **Matching controller creation order**: The order of port/address pairs passed to
   ``start_bumble_controllers`` determines which executable connects to which address.

3. **Connecting executables to ports**: Test executables connect to specific ports,
   thereby inheriting the associated BD address.

**Example: GAP Discovery Test**

In the GAP discovery test, the peripheral role is assigned a specific BD address and the
central is informed of this address via command-line parameter:

**Test Script** (:zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/tests_scripts/discovery.sh`):

.. code-block:: bash

   # Allocate ports
   HCI_PORT_0=$(allocate_tcp_port)
   HCI_PORT_1=$(allocate_tcp_port)

   # Create controllers with port-to-address mapping
   # BD_ADDR[0] = "00:00:01:00:00:01" (peripheral)
   # BD_ADDR[1] = "00:00:01:00:00:02" (central)
   BUMBLE_CONTROLLER_PID=$(start_bumble_controllers \
       "$BUMBLE_CONTROLLERS_LOG" \
       "$HCI_PORT_0@${BD_ADDR[0]}" \
       "$HCI_PORT_1@${BD_ADDR[1]}")

   # Connect peripheral executable to HCI_PORT_0
   # This gives peripheral the BD address 00:00:01:00:00:01
   # Pass central's address as peer_bd_address parameter
   test_case_name="bluetooth.classic.gap.discovery"
   peripheral_exe="${BT_CLASSIC_BIN}/bt_classic_${BOARD_SANITIZED}_${test_case_name}.exe"
   ARGS="--peer_bd_address=${BD_ADDR[1]} -test=gap_peripheral::*"
   execute_async "$peripheral_exe" --bt-dev="127.0.0.1:$HCI_PORT_0" "$ARGS" || exit 1

   # Connect central executable to HCI_PORT_1
   # This gives central the BD address 00:00:01:00:00:02
   # Pass peripheral's address as peer_bd_address parameter
   test_case_name="bluetooth.classic.gap.discovery"
   central_exe="${BT_CLASSIC_BIN}/bt_classic_${BOARD_SANITIZED}_${test_case_name}.exe"
   ARGS="--peer_bd_address=${BD_ADDR[0]}  -test=gap_central::*"
   execute_async "$central_exe" --bt-dev="127.0.0.1:$HCI_PORT_1" "$ARGS" || exit 1


In this example:

- The peripheral executable connects to ``HCI_PORT_0``, which is bound to BD address
  ``00:00:01:00:00:01``
- The central executable connects to ``HCI_PORT_1``, which is bound to BD address
  ``00:00:01:00:00:02``
- The peripheral receives the central's address ``00:00:01:00:00:02`` via the
  ``--peer_bd_address`` parameter
- The central receives the peripheral's address ``00:00:01:00:00:01`` via the
  ``--peer_bd_address`` parameter
- The ``-test`` parameter specifies which test suite to run (``gap_peripheral::*``
  or ``gap_central::*``)

This mapping allows test applications to:

- Know the BD addresses of peer devices in advance
- Configure expected peer addresses via runtime parameters
- Verify discovery results against known addresses
- Establish connections to specific peer devices
- Run different test suites from the same executable

Best Practices
--------------

When writing new tests:

1. **Use sequential BD addresses**: Assign ``BD_ADDR[0]`` to the first device,
   ``BD_ADDR[1]`` to the second, etc.

2. **Document role-to-address mapping**: Clearly document which role uses which BD address
   in test documentation and comments.

3. **Pass peer addresses as parameters**: Use ``--peer_bd_address`` parameter to inform
   devices about their peers' addresses at runtime.

4. **Use test suite selection**: Use ``-test`` parameter to specify which test suite to run
   from a single executable.

5. **Maintain consistent ordering**: Keep the same order when creating controllers and
   launching executables to maintain the role-to-address mapping.

6. **Allocate ports sequentially**: Allocate ports one by one in the order they will be
   used to maintain clarity in the test script.

Directory Structure
*******************

.. code-block:: none

   tests/bluetooth/classic/sim/
   ├── ci.bt.classic.sh                    # Main CI entry point
   ├── ci.bt.classic.native_sim.compile.sh # Compilation script for CI
   ├── ci.bt.classic.native_sim.run.sh     # Test execution script for CI
   ├── common/                             # Shared utilities and controllers
   │   ├── controllers.py                  # Simulated controller (Bumble 0.0.220)
   │   └── common_utils.sh               # Main utility entry point (includes all helpers)
   └── gap_discovery/                      # Example test: GAP discovery
       ├── src/                            # Test source code
       │   ├── test_main.c                 # Main entry point with parameter parsing
       │   ├── test_central.c              # Central role test implementation
       │   └── test_peripheral.c           # Peripheral role test implementation
       ├── tests_scripts/                  # Test execution scripts
       │   └── discovery.sh                # Discovery test script
       ├── CMakeLists.txt                  # CMake build configuration
       ├── compile.sh                      # Build script for this test
       ├── Kconfig                         # Kconfig options
       ├── prj.conf                        # Project configuration
       ├── README.rst                      # Test documentation
       └── testcase.yaml                   # Test case definitions

Test Registry File
******************

The framework uses a central registry file to manage which tests should be compiled and
executed. This file is located at:

:zephyr_file:`tests/bluetooth/classic/sim/tests.native_sim.txt`

Purpose
=======

The ``tests.native_sim.txt`` file serves as a registry of all Bluetooth Classic simulation
tests that should be included in the build and test execution process.

File Format
===========

The file uses a simple text format with the following rules:

- **One test directory per line**: Each line specifies a test directory to include
- **Comments**: Lines starting with ``#`` are treated as comments and ignored
- **Empty lines**: Blank lines are ignored
- **Relative paths**: Paths are relative to the ``tests/bluetooth/classic/sim/`` directory
- **Absolute paths**: Absolute paths are also supported (though not commonly used)

Example Content
===============

.. code-block:: text

   # Copyright 2025 NXP
   # SPDX-License-Identifier: Apache-2.0
   #
   # Bluetooth Classic Simulation Test Directories
   # Lines starting with # are comments
   # Empty lines are ignored
   # Paths can be relative (to this script's directory) or absolute

   # GAP Discovery tests
   gap_discovery

   # Add more test directories here
   # another_test
   # some_other_test

   # You can also use absolute paths
   # /absolute/path/to/test/directory

Usage in CI Scripts
===================

The CI compilation script (:zephyr_file:`tests/bluetooth/classic/sim/ci.bt.classic.native_sim.compile.sh`)
reads this file to determine which tests to build.

Similarly, the test execution script (:zephyr_file:`tests/bluetooth/classic/sim/ci.bt.classic.native_sim.run.sh`)
uses this file to determine which test scripts to execute.

Adding a New Test
=================

To add a new test to the framework:

1. **Create the test directory** with all required files (see :ref:`Writing New Tests <bluetooth_classic_sim>`)

2. **Add an entry to tests.native_sim.txt**:

   .. code-block:: text

      # My new test
      my_new_test

3. **Verify the test is included**:

   .. code-block:: bash

      # The test should now be compiled and executed by CI scripts
      ${ZEPHYR_BASE}/tests/bluetooth/classic/sim/ci.bt.classic.native_sim.compile.sh
      ${ZEPHYR_BASE}/tests/bluetooth/classic/sim/ci.bt.classic.native_sim.run.sh

Temporarily Disabling Tests
============================

To temporarily disable a test without removing it from the repository:

1. **Comment out the test entry**:

   .. code-block:: text

      # GAP Discovery tests
      gap_discovery

      # Temporarily disabled - investigating timeout issues
      # my_flaky_test

      # Another test
      another_test

2. **The test will be skipped** during compilation and execution by CI scripts

Best Practices
==============

When managing the test registry:

1. **Add comments**: Include descriptive comments for each test or group of tests

2. **Group related tests**: Organize tests by feature area or test type

3. **Document disabled tests**: When commenting out a test, add a comment explaining why

4. **Keep it sorted**: Consider maintaining alphabetical or logical ordering for easier navigation

5. **Use relative paths**: Prefer relative paths over absolute paths for portability

6. **One test per line**: Don't combine multiple tests on a single line

7. **Verify before committing**: Always verify that tests compile and run after modifying this file

Example with Multiple Tests
============================

.. code-block:: text

   # Copyright 2025 NXP
   # SPDX-License-Identifier: Apache-2.0

   # GAP (Generic Access Profile) Tests
   gap_discovery
   gap_connection
   gap_pairing

   # SDP (Service Discovery Protocol) Tests
   sdp_server
   sdp_client

   # RFCOMM Tests
   rfcomm_basic
   # rfcomm_advanced  # TODO: Fix timing issues

   # L2CAP Tests
   l2cap_basic

   # Profile Tests
   profile_hfp
   profile_a2dp

Location in Build Process
==========================

The test registry file is processed at two key stages:

1. **Compilation Stage**: The ``ci.bt.classic.native_sim.compile.sh`` script reads this
   file to determine which test directories to build using ``west twister``.

2. **Execution Stage**: The ``ci.bt.classic.native_sim.run.sh`` script reads this file
   to determine which test scripts (in ``tests_scripts/`` subdirectories) to execute.

This two-stage approach ensures that only registered tests are built and executed,
providing a clean and manageable test suite.

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

To build all Bluetooth Classic simulation tests:

.. code-block:: bash

   ${ZEPHYR_BASE}/tests/bluetooth/classic/sim/ci.bt.classic.native_sim.compile.sh

To build a specific test (e.g., GAP discovery):

.. code-block:: bash

   ${ZEPHYR_BASE}/tests/bluetooth/classic/sim/gap_discovery/compile.sh

The build process:

1. Uses ``west twister`` to build test applications for the :zephyr:board:`native_sim` board
2. Extracts the built executables (``zephyr.exe``)
3. Copies them to a central binary directory with standardized names

Running Tests
=============

To run all tests:

.. code-block:: bash

   ${ZEPHYR_BASE}/tests/bluetooth/classic/sim/ci.bt.classic.native_sim.run.sh

To run a specific test:

.. code-block:: bash

   ${ZEPHYR_BASE}/tests/bluetooth/classic/sim/gap_discovery/tests_scripts/discovery.sh

Test Results
============

Test results are generated in JUnit XML format, suitable for CI/CV integration. The results
file location can be specified via the ``RESULTS_FILE`` environment variable.

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
   ├── tests_scripts/             # Test execution scripts
   │   └── test_scenario.sh       # Test orchestration script
   ├── CMakeLists.txt             # CMake build configuration
   ├── compile.sh                 # Build script
   ├── Kconfig                    # Kconfig options (optional)
   ├── prj.conf                   # Project configuration
   ├── README.rst                 # Test documentation
   └── testcase.yaml              # Test case definitions

Tests use separate source files for different device roles, with each role implementing
its test logic using the Ztest framework. A main entry point (``test_main.c``) handles
parameter parsing and test suite selection.

Build Script (compile.sh)
=========================

The build script should source ``common_utils.sh`` which provides all necessary utility functions:

.. code-block:: bash

   #!/usr/bin/env bash
   # Copyright 2026 NXP
   # SPDX-License-Identifier: Apache-2.0

   set -ue

   : "${ZEPHYR_BASE:?ZEPHYR_BASE must be set}"

   SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
   TEST_NAME="$(basename "${SCRIPT_DIR}")"
   COMMON_DIR="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/common"

   # Source common_utils.sh - this is the main entry point for all utilities
   source "$COMMON_DIR/common_utils.sh"

   BUILD_TEMP_DIR=$(create_temp_dir "${TEST_NAME}_build")

   # Build all test applications
   west twister -p "${BOARD}" -T "${SCRIPT_DIR}" -O "${BUILD_TEMP_DIR}" --build-only
   extra_twister_out "${BUILD_TEMP_DIR}" "${TEST_NAME}"

Test Case Definition (testcase.yaml)
====================================

The ``testcase.yaml`` file defines the test scenarios. With the updated approach, a single
test case can run multiple test suites via runtime parameters:

.. code-block:: yaml

   common:
     tags: bluetooth classic
     platform_allow:
       - native_sim
     integration_platforms:
       - native_sim

   tests:
     bluetooth.classic.my_test:
       # Single test case, test suite selected at runtime via -test parameter

CMakeLists.txt Configuration
=============================

The ``CMakeLists.txt`` file should include all source files:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.20.0)
   find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
   project(bt_classic_gap_discovery)

   target_sources(app PRIVATE
     src/test_main.c
     src/test_central.c
     src/test_peripheral.c
   )

Test Script (tests_scripts/test_scenario.sh)
=============================================

Test scripts should source ``common_utils.sh`` which provides all necessary functions
including port management, controller management, and async execution:

.. code-block:: bash

   #!/usr/bin/env bash
   # Copyright 2026 NXP
   # SPDX-License-Identifier: Apache-2.0

   set -ue

   COMMON_DIR="${ZEPHYR_BASE}/tests/bluetooth/classic/sim/common"

   # Source common_utils.sh - this provides all utility functions
   source "$COMMON_DIR/common_utils.sh"

   SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
   TEST_NAME="$(basename "$(dirname "$SCRIPT_DIR")")"
   TEST_OUTPUT_DIR="${BT_CLASSIC_SIM}/${TEST_NAME}"
   mkdir -p "$TEST_OUTPUT_DIR"

   BUMBLE_CONTROLLERS_LOG="$TEST_OUTPUT_DIR/bumble_controllers.log"

   # Initialize cleanup variables
   HCI_PORT_0=""
   HCI_PORT_1=""
   BUMBLE_CONTROLLER_PID=""

   # Cleanup function
   cleanup() {
       local exit_code=$?

       if [[ -f "$BUMBLE_CONTROLLERS_LOG" ]]; then
           cat "$BUMBLE_CONTROLLERS_LOG"
       fi

       if [[ -n "$BUMBLE_CONTROLLER_PID" ]]; then
           stop_bumble_controllers $BUMBLE_CONTROLLER_PID || true
       fi

       if [[ -n "$HCI_PORT_0" ]]; then
           release_tcp_port $HCI_PORT_0 || true
       fi

       if [[ -n "$HCI_PORT_1" ]]; then
           release_tcp_port $HCI_PORT_1 || true
       fi

       exit $exit_code
   }

   trap cleanup EXIT

   # Allocate TCP ports for HCI communication (one by one)
   HCI_PORT_0=$(allocate_tcp_port)
   if [[ -z "$HCI_PORT_0" ]]; then
       exit 1
   fi

   HCI_PORT_1=$(allocate_tcp_port)
   if [[ -z "$HCI_PORT_1" ]]; then
       exit 1
   fi

   # Start simulated Bluetooth controllers (Bumble 0.0.220 based)
   BUMBLE_CONTROLLER_PID=$(start_bumble_controllers \
       "$BUMBLE_CONTROLLERS_LOG" \
       "$HCI_PORT_0@${BD_ADDR[0]}" "$HCI_PORT_1@${BD_ADDR[1]}")

   if [[ -z "$BUMBLE_CONTROLLER_PID" ]]; then
       echo "Error: Failed to start bumble controllers" >&2
       exit 1
   fi

   # Run test applications with test suite selection
   test_case_name="bluetooth.classic.my_test"
   test_exe="${BT_CLASSIC_BIN}/bt_classic_${BOARD_SANITIZED}_${test_case_name}.exe"

   # Run role1 with its test suite
   execute_async "$test_exe" --bt-dev="127.0.0.1:$HCI_PORT_0" -test=role1_suite \
       --peer_bd_address="${BD_ADDR[1]}" || exit 1

   # Run role2 with its test suite and peer address
   execute_async "$test_exe" --bt-dev="127.0.0.1:$HCI_PORT_1" -test=role2_suite \
       --peer_bd_address="${BD_ADDR[0]}" || exit 1

   # Wait for all test applications to complete
   wait_for_async_jobs

Utility Functions
=================

By sourcing ``common_utils.sh``, the following utility functions become available:

**Port Management:**

* ``allocate_tcp_port`` - Allocate a free TCP port
* ``release_tcp_port <port>`` - Release an allocated TCP port

**Controller Management:**

* ``start_bumble_controllers <log_file> <port1@addr1> [port2@addr2] ...`` - Start Bumble controllers
* ``stop_bumble_controllers <pid>`` - Stop running controllers

**Async Execution:**

* ``execute_async <executable> [args...]`` - Run executable asynchronously
* ``wait_for_async_jobs`` - Wait for all async jobs to complete

**Build Utilities:**

* ``create_temp_dir <prefix>`` - Create temporary build directory
* ``extra_twister_out <twister_dir> <test_name>`` - Extract and copy built executables

Test Application Code
=====================

Test applications use the Ztest framework for test case definition and execution. The main
entry point handles parameter parsing and test suite selection.

**Main Entry Point** (``test_main.c``):

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <zephyr/shell/shell.h>
   #include <zephyr/logging/log.h>
   #include <string.h>

   LOG_MODULE_REGISTER(test_main, LOG_LEVEL_DBG);

   /* Global variable to store peer BD address */
   char peer_bd_address[18] = {0};

   /* Parse peer_bd_address parameter */
   static int parse_peer_address(const struct shell *sh, size_t argc, char **argv)
   {
       for (size_t i = 0; i < argc; i++) {
           if (strncmp(argv[i], "--peer_bd_address=", 18) == 0) {
               strncpy(peer_bd_address, argv[i] + 18, sizeof(peer_bd_address) - 1);
               LOG_INF("Peer BD address: %s", peer_bd_address);
               return 0;
           }
       }
       return -1;
   }

   int main(void)
   {
       /* Parse command line arguments */
       const struct shell *sh = shell_backend_uart_get_ptr();
       if (sh) {
           parse_peer_address(sh, sh->ctx->argc, sh->ctx->argv);
       }

       /* Ztest will run the appropriate test suite based on -test parameter */
       return 0;
   }

**Central Role Example** (``test_central.c``):

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <zephyr/bluetooth/bluetooth.h>
   #include <zephyr/bluetooth/hci.h>
   #include <zephyr/logging/log.h>
   #include <zephyr/ztest.h>

   LOG_MODULE_REGISTER(test_gap_central, LOG_LEVEL_DBG);

   /* External reference to peer address from test_main.c */
   extern char peer_bd_address[18];

   static K_SEM_DEFINE(discovery_sem, 0, 1);
   static bt_addr_t peer_addr;

   static void parse_bd_address(const char *str, bt_addr_t *addr)
   {
       /* Parse BD address string to bt_addr_t */
       sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
              &addr->val[5], &addr->val[4], &addr->val[3],
              &addr->val[2], &addr->val[1], &addr->val[0]);
   }

   static void br_discover_recv(const struct bt_br_discovery_result *result)
   {
       if (bt_addr_eq(&peer_addr, &result->addr)) {
           LOG_DBG("Target device found");
           k_sem_give(&discovery_sem);
       }
   }

   static struct bt_br_discovery_cb br_discover = {
       .recv = br_discover_recv,
   };

   ZTEST(gap_central, test_01_gap_central_general_discovery)
   {
       int err;
       struct bt_br_discovery_param param = {
           .length = 10,
           .limited = false,
       };

       err = bt_br_discovery_start(&param, NULL, 0);
       zassert_equal(err, 0, "Discovery start failed (err %d)", err);

       err = k_sem_take(&discovery_sem, K_SECONDS(30));
       zassert_equal(err, 0, "Discovery timeout (err %d)", err);

       bt_br_discovery_stop();
   }

   static void *setup(void)
   {
       int err;

       err = bt_enable(NULL);
       zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);

       /* Parse peer address from command line parameter */
       parse_bd_address(peer_bd_address, &peer_addr);
       LOG_INF("Looking for peer: %s", peer_bd_address);

       bt_br_discovery_cb_register(&br_discover);

       return NULL;
   }

   static void teardown(void *f)
   {
       bt_disable();
   }

   ZTEST_SUITE(gap_central, NULL, setup, NULL, NULL, teardown);

**Peripheral Role Example** (``test_peripheral.c``):

.. code-block:: c

   #include <zephyr/kernel.h>
   #include <zephyr/bluetooth/bluetooth.h>
   #include <zephyr/bluetooth/hci.h>
   #include <zephyr/logging/log.h>
   #include <zephyr/ztest.h>

   LOG_MODULE_REGISTER(test_gap_peripheral, LOG_LEVEL_DBG);

   static K_SEM_DEFINE(connection_sem, 0, 1);

   static void br_connected(struct bt_conn *conn, uint8_t conn_err)
   {
       if (conn_err == 0) {
           k_sem_give(&connection_sem);
       }
   }

   BT_CONN_CB_DEFINE(conn_callbacks) = {
       .connected = br_connected,
   };

   ZTEST(gap_peripheral, test_01_gap_peripheral_general_discovery)
   {
       int err;

       err = bt_br_set_connectable(true, NULL);
       zassert_equal(err, 0, "Failed to set connectable (err %d)", err);

       err = bt_br_set_discoverable(true, false);
       zassert_equal(err, 0, "Failed to set discoverable (err %d)", err);

       err = k_sem_take(&connection_sem, K_SECONDS(60));
       zassert_equal(err, 0, "Connection timeout (err %d)", err);

       bt_br_set_discoverable(false, false);
       bt_br_set_connectable(false, NULL);
   }

   static void *setup(void)
   {
       int err;

       err = bt_enable(NULL);
       zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);

       return NULL;
   }

   static void teardown(void *f)
   {
       bt_disable();
   }

   ZTEST_SUITE(gap_peripheral, NULL, setup, NULL, NULL, teardown);

Key Points for Test Applications
=================================

* **Ztest Framework**: Use ``ZTEST()`` macro to define test cases
* **Test Suites**: Use ``ZTEST_SUITE()`` to define test suites with setup/teardown
* **Test Suite Selection**: Use ``-test`` parameter to select which suite to run at runtime
* **Parameter Parsing**: Implement parameter parsing in ``test_main.c`` for runtime configuration
* **Peer Address**: Use ``--peer_bd_address`` parameter to pass peer device addresses
* **Assertions**: Use ``zassert_*()`` macros for test assertions
* **Synchronization**: Use semaphores or other synchronization primitives for inter-device coordination
* **Logging**: Use the logging subsystem for debug output
* **Timeouts**: Always use appropriate timeouts to prevent hanging tests
* **Cleanup**: Ensure proper cleanup in teardown functions

Conventions
***********

Test Scripts
============

* **Utility Import**: Always source ``common_utils.sh`` - it is the single entry point for all utilities
* **Naming**: Test scripts must have the ``.sh`` extension and be placed in ``tests_scripts/`` subdirectory
* **Exit Codes**: Return 0 for success, non-zero for failure
* **Resource Management**: Always clean up allocated resources (ports, processes) using trap handlers
* **Logging**: Output should be concise; detailed logs can be written to files
* **Timeouts**: Set appropriate timeouts to prevent hanging tests
* **Port Allocation**: Use ``allocate_tcp_port`` function to obtain free TCP ports one by one
* **Port Release**: Release all allocated ports in cleanup handlers using ``release_tcp_port``
* **Controller Startup**: Use ``start_bumble_controllers`` to create simulated Bluetooth Classic controllers
* **Async Execution**: Run test executables asynchronously using ``execute_async``
* **Synchronization**: Wait for all async jobs to complete using ``wait_for_async_jobs``
* **Test Suite Selection**: Use ``-test`` parameter to specify which test suite to run
* **Peer Address Passing**: Use ``--peer_bd_address`` parameter to pass peer device addresses

Port Allocation
===============

* Use ``allocate_tcp_port`` to obtain free TCP ports
* Always release ports in cleanup handlers using ``release_tcp_port``
* Ports are used for HCI communication between test applications and simulated controllers

Controller Management
=====================

* Controllers are based on Bumble 0.0.220
* Use ``start_bumble_controllers`` to launch simulated controllers
* Specify Bluetooth addresses using the format: ``PORT@BD_ADDR``
* Always stop controllers in cleanup handlers using ``stop_bumble_controllers``
* The number of controllers is determined by the number of port/address pairs provided

Parallel Execution
==================

* Tests can run in parallel using GNU parallel (if available)
* Each test must use unique TCP ports to avoid conflicts
* The framework automatically handles parallel execution when running multiple tests

CI Integration
**************

The Bluetooth Classic simulation tests are integrated into the CI pipeline via:

.. code-block:: bash

   tests/bluetooth/classic/sim/ci.bt.classic.sh

This script:

1. Compiles all test applications
2. Runs all test scripts in parallel
3. Generates JUnit XML test results
4. Returns appropriate exit codes for CI systems

Example: GAP Discovery Test
****************************

The GAP discovery test demonstrates device discovery functionality.

Test Scenario
=============

The GAP discovery test includes multiple test cases:

1. **General Discovery**:

   * Peripheral enables general discoverable mode
   * Central performs general inquiry
   * Central discovers peripheral
   * Devices establish connection

2. **Limited Discovery**:

   * Peripheral enables limited discoverable mode
   * Central performs limited inquiry
   * Central discovers peripheral with limited bit set
   * After timeout, limited mode expires
   * Test verifies limited bit is cleared

Files
=====

* :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/src/test_main.c` - Main entry point with parameter parsing
* :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/src/test_central.c` - Central role test implementation
* :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/src/test_peripheral.c` - Peripheral role test implementation
* :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/tests_scripts/discovery.sh` - Test orchestration
* :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/testcase.yaml` - Test case definitions
* :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/Kconfig` - Kconfig options
* :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/README.rst` - Detailed test documentation

Test Script Flow
================

1. Source ``common_utils.sh`` to load all utilities
2. Allocate two TCP ports using ``allocate_tcp_port`` (one for each device)
3. Start Bumble controllers with ``start_bumble_controllers``
4. Launch peripheral executable asynchronously with ``execute_async`` and ``-test=gap_peripheral``, and ``--peer_bd_address``
5. Launch central executable asynchronously with ``execute_async``, ``-test=gap_central``, and ``--peer_bd_address``
6. Wait for completion with ``wait_for_async_jobs``
7. Cleanup releases ports and stops controllers via trap handler

Test Execution
==============

A single executable runs different test suites based on the ``-test`` parameter:

* **Peripheral**: Runs with ``-test=gap_peripheral`` and ``--peer_bd_address=${BD_ADDR[0]}`` to execute the ``gap_peripheral`` test suite
* **Central**: Runs with ``-test=gap_central`` and ``--peer_bd_address=${BD_ADDR[0]}`` to execute the ``gap_central`` test suite

The tests synchronize through Bluetooth operations (discovery, connection) rather than
direct inter-process communication.

Troubleshooting
***************

Port Allocation Failures
=========================

If tests fail with port allocation errors:

* Check for stale processes holding ports: ``netstat -tulpn | grep <port>``
* Ensure cleanup handlers are properly releasing ports
* Verify no port conflicts with other services

Controller Startup Failures
============================

If simulated controllers fail to start:

* Verify Bumble version: ``pip show bumble`` (should be 0.0.220)
* Check Python dependencies: ``pip list | grep bumble``
* Review controller logs in ``$TEST_OUTPUT_DIR/bumble_controllers.log``
* Verify TCP ports are accessible

Test Timeouts
=============

If tests hang or timeout:

* Check application logs for blocking operations
* Verify proper synchronization between devices
* Ensure cleanup handlers can execute (avoid infinite loops)
* Check that both devices are running and can communicate
* Verify the correct test suite is being selected with ``-test`` parameter

Bumble Version Mismatch
=======================

If you encounter compatibility issues:

* Ensure you have exactly Bumble 0.0.220 installed: ``pip install bumble==0.0.220``
* Newer or older versions may have incompatible APIs

Ztest Assertion Failures
=========================

If Ztest assertions fail:

* Review test logs for specific assertion failures
* Check timing between central and peripheral operations
* Verify Bluetooth addresses are correctly passed via ``--peer_bd_address``
* Ensure devices are in the correct state before operations
* Verify the correct test suite is running (check ``-test`` parameter)

Parameter Parsing Issues
=========================

If parameters are not being recognized:

* Verify parameter format: ``--peer_bd_address=XX:XX:XX:XX:XX:XX``
* Check that ``test_main.c`` properly parses command-line arguments
* Ensure the shell backend is available for parameter parsing
* Review logs to confirm parameters are being received

Test Suite Selection Issues
============================

If the wrong test suite is running:

* Verify the ``-test`` parameter is correctly specified in the test script
* Check that test suite names match between the script and test code
* Ensure Ztest is properly configured to recognize the ``-test`` parameter
* Review test output to confirm which suite is executing
