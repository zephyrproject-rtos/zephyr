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

* A Bumble-based controller simulator (:zephyr_file:`tests/bluetooth/classic/sim/common/controllers.py`)
* Shell utilities for port allocation, controller process management, and async execution
  (imported via :zephyr_file:`tests/bluetooth/classic/sim/common/common_utils.sh`)
* One or more :zephyr:board:`native_sim` test executables launched by Twister's ``script`` harness

Architecture
************

The Bluetooth Classic simulation framework consists of several key components:

Simulated Controllers
=====================

The framework uses Python-based simulated Bluetooth Classic controllers built on
**Bumble 0.0.220**. These controllers implement the HCI interface over TCP and provide:

* **BR/EDR Support**: Full Bluetooth Classic (BR/EDR) functionality
* **HCI Interface**: Standard HCI communication over TCP
* **Multiple Controllers**: Support for running multiple simulated devices simultaneously
* **Link Simulation**: Simulated radio links between controllers for device discovery and connections
* **Bumble-based**: Leverages the Bumble Bluetooth stack (version 0.0.220) for protocol implementation

The controller implementation can be found in:

* :zephyr_file:`tests/bluetooth/classic/sim/common/controllers.py` - Main controller simulator based on Bumble 0.0.220

Related simulation building blocks (used by the controller implementation) live alongside it:

* :zephyr_file:`tests/bluetooth/classic/sim/common/bt_sim_controller.py`
* :zephyr_file:`tests/bluetooth/classic/sim/common/bt_sim_hci.py`
* :zephyr_file:`tests/bluetooth/classic/sim/common/bt_sim_link.py`
* :zephyr_file:`tests/bluetooth/classic/sim/common/bt_sim_ll.py`

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
   ``--peer_bd_address=XX:XX:XX:XX:XX:XX`` parameter to specify the expected peer device
   address.

   In the Bluetooth Classic simulation tests (running on :zephyr:board:`native_sim`), the
   parameter is parsed using the native_sim command line options framework
   (see :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/src/test_main.c`).

2. **Matching controller creation order**: The order of port/address pairs passed to
   ``start_bumble_controllers`` determines which executable connects to which address.

3. **Connecting executables to ports**: Test executables connect to specific ports,
   thereby inheriting the associated BD address.

**Example: GAP Discovery Test**

In the GAP discovery test, the peripheral role is assigned a specific BD address and the
central is informed of this address via the ``--peer_bd_address`` command-line parameter
(parsed by the native_sim cmdline framework in
:zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/src/test_main.c`):

**Test Script** (:zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/tests_scripts/general_discovery.sh`):

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

   test_case_name="bluetooth.classic.sim.gap.discovery"
   zephyr_exe=$(extra_twister_out "${ZEPHYR_BASE}/twister-out" "${test_case_name}")

   # Connect peripheral executable to HCI_PORT_0
   # This gives peripheral the BD address 00:00:01:00:00:01
   # Pass central's address as peer_bd_address parameter
   peripheral_exe="${zephyr_exe}"
   ARGS=("--peer_bd_address=${BD_ADDR[1]}" \
         "-test=gap_peripheral::test_01_general_discovery")
   execute_async "$peripheral_exe" --bt-dev="127.0.0.1:$HCI_PORT_0" "${ARGS[@]}" || exit 1

   # Connect central executable to HCI_PORT_1
   # This gives central the BD address 00:00:01:00:00:02
   # Pass peripheral's address as peer_bd_address parameter
   central_exe="${zephyr_exe}"
   ARGS=("--peer_bd_address=${BD_ADDR[0]}" \
         "-test=gap_central::test_01_general_discovery")
   execute_async "$central_exe" --bt-dev="127.0.0.1:$HCI_PORT_1" "${ARGS[@]}" || exit 1


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

Test execution (Twister + script harness)
=========================================

The Bluetooth Classic simulation tests are standard Twister tests that use the ``script``
harness.

* Each test directory provides its own :file:`tests.yaml` (for example
  :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/tests.yaml`).
* Twister launches the listed ``tests_scripts/*.sh`` scripts.
* The script starts one or more Bumble-based controllers and then launches one or more
  instances of the built ``native_sim`` executable.

Run the whole suite:

.. code-block:: console

   west twister -T tests/bluetooth/classic/sim

Run a single test:

.. code-block:: console

   west twister -T tests/bluetooth/classic/sim/gap_discovery

Run the script harness directly:

.. code-block:: console

   ./tests/bluetooth/classic/sim/gap_discovery/tests_scripts/general_discovery.sh

Directory Structure
*******************

.. code-block:: none

   tests/bluetooth/classic/sim/
   ├── common/                             # Shared utilities and controllers
   │   ├── controllers.py                  # Simulated controller (Bumble-based)
   │   ├── common_utils.sh                 # One-stop entrypoint (sources all helpers)
   │   ├── bumble_utils.sh                 # Controller process management helpers
   │   ├── port_utils.sh                   # Thread-safe TCP port allocation
   │   ├── parameter_utils.sh              # Predefined BD_ADDR[] and other params
   │   └── thread_utils.sh                 # Process/thread helpers (kill/timeouts, etc.)
   └── gap_discovery/                      # Example test: GAP discovery
       ├── src/                            # Test source code
       │   ├── test_main.c                 # native_sim cmdline parsing + suite selection
       │   ├── test_central.c              # Central role tests
       │   └── test_peripheral.c           # Peripheral role tests
       ├── tests_scripts/                  # Test execution scripts
       │   └── general_discovery.sh        # Script-harness entrypoint
       ├── CMakeLists.txt                  # CMake build configuration
       ├── Kconfig                         # Kconfig options
       ├── prj.conf                        # Project configuration
       ├── README.rst                      # Test documentation
       └── tests.yaml                      # Test case definitions

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

   west twister -T tests/bluetooth/classic/sim --build-only

Build a specific test:

.. code-block:: console

   west twister -T tests/bluetooth/classic/sim/gap_discovery --build-only

Running Tests
=============

Run via Twister:

.. code-block:: console

   west twister -T tests/bluetooth/classic/sim

Or run a specific script harness directly:

.. code-block:: console

   ./tests/bluetooth/classic/sim/gap_discovery/tests_scripts/general_discovery.sh
   ./tests/bluetooth/classic/sim/gap_discovery/tests_scripts/limited_discovery.sh

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
   ├── Kconfig                    # Kconfig options (optional)
   ├── prj.conf                   # Project configuration
   ├── README.rst                 # Test documentation
   └── tests.yaml                 # Test case definitions

Tests use separate source files for different device roles, with each role implementing
its test logic using the Ztest framework. A main entry point (``test_main.c``) handles
parameter parsing and test suite selection.

Test Case Definition (tests.yaml)
====================================

The ``tests.yaml`` file defines the test scenarios. With the updated approach, a single
test case can run multiple test suites via runtime parameters:

.. code-block:: yaml

   common:
     tags: bluetooth classic
     platform_allow:
       - native_sim
     integration_platforms:
       - native_sim
     harness: script

   tests:
     bluetooth.classic.my_test:
       harness_config:
         tests_scripts:
           - tests_scripts/test_scenario.sh

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
   # Get the built executable from Twister output for this test case
   test_case_name="bluetooth.classic.my_test"
   zephyr_exe=$(extra_twister_out "${ZEPHYR_BASE}/twister-out" "${test_case_name}")

   # Run role1 with its test suite
   execute_async "${zephyr_exe}" --bt-dev="127.0.0.1:$HCI_PORT_0" -test=role1_suite \
       --peer_bd_address="${BD_ADDR[1]}" || exit 1

   # Run role2 with its test suite and peer address
   execute_async "${zephyr_exe}" --bt-dev="127.0.0.1:$HCI_PORT_1" -test=role2_suite \
       --peer_bd_address="${BD_ADDR[0]}" || exit 1

   # Wait for all test applications to complete
   wait_for_async_jobs

Utility Functions
=================

By sourcing :zephyr_file:`tests/bluetooth/classic/sim/common/common_utils.sh`, the
following utility functions become available (re-exported from the sourced helpers):

**Port Management:**

* ``allocate_tcp_port`` - Allocate a free TCP port
* ``release_tcp_port <port>`` - Release an allocated TCP port

Implementation details:

* Port allocation helpers: :zephyr_file:`tests/bluetooth/classic/sim/common/port_utils.sh`

**Controller Management:**

* ``start_bumble_controllers <log_file> <port1@addr1> [port2@addr2] ...`` - Start Bumble controllers
* ``stop_bumble_controllers <pid>`` - Stop running controllers

Implementation details:

* Controller process helpers: :zephyr_file:`tests/bluetooth/classic/sim/common/bumble_utils.sh`

**Async Execution:**

* ``execute_async <executable> [args...]`` - Run executable asynchronously
* ``wait_for_async_jobs`` - Wait for all async jobs to complete

Implementation details:

* Async helpers: :zephyr_file:`tests/bluetooth/classic/sim/common/thread_utils.sh`

**Build Utilities:**

* ``create_temp_dir <prefix>`` - Create temporary build directory
* ``extra_twister_out <twister_out_dir> <test_name>`` - Find and return the matching
  ``zephyr.exe`` under a Twister output directory. Selection rules:

  * searches for all ``zephyr.exe`` files under ``twister_out_dir``
  * filters candidates to only those whose path contains ``/${test_name}/`` (full directory
    component match)
  * if multiple candidates match, picks the newest (mtime) and prints a warning
  * prints the chosen path to **stdout** (so scripts can capture it), and logs info/warnings
    to **stderr**

Implementation details:

* Build output selection helper: :zephyr_file:`tests/bluetooth/classic/sim/common/common_utils.sh`
* Parameter utilities (including the predefined ``BD_ADDR[]`` array):
  :zephyr_file:`tests/bluetooth/classic/sim/common/parameter_utils.sh`

Test Application Code
=====================

Test applications use the Ztest framework for test case definition and execution.

For :zephyr:board:`native_sim` based tests, command line arguments are registered using the
native_sim command line option framework (``cmdline.h``). For example, the GAP discovery test
registers a mandatory ``peer_bd_address`` string option and parses it using
``bt_addr_from_str()``:

- :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/src/test_main.c`

.. code-block:: c

   /* native_sim command line options header */
   #include "cmdline.h" /* native_sim command line options header */

   extern bt_addr_t peer_addr;

   static void cmd_peer_bd_address_found(char *argv, int offset)
   {
       char *addr_str = &argv[offset];
       int err;

       err = bt_addr_from_str(addr_str, &peer_addr);
       if (err != 0) {
           LOG_ERR("Failed to parse peer Bluetooth address: %s (err %d)", addr_str, err);
       }
   }

   static void gap_discovery_args(void)
   {
       static struct args_struct_t args[] = {
           {
               .manual = false,
               .is_mandatory = true,
               .is_switch = false,
               .option = "peer_bd_address",
               .name = "XX:XX:XX:XX:XX:XX",
               .type = 's',
               .dest = NULL,
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

   LOG_MODULE_REGISTER(test_gap_central, LOG_LEVEL_DBG);

   /* External reference to peer address from test_main.c */
   extern bt_addr_t peer_addr;

   static K_SEM_DEFINE(discovery_sem, 0, 1);

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

   ZTEST(gap_central, test_01_general_discovery)
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

       /* peer_addr is parsed from --peer_bd_address by the native_sim cmdline framework */
       LOG_INF("Looking for peer: %s", bt_addr_str(&peer_addr));

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

   ZTEST(gap_peripheral, test_01_general_discovery)
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
* :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/tests_scripts/general_discovery.sh` - Test orchestration
* :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/tests_scripts/limited_discovery.sh` - Test orchestration
* :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/tests.yaml` - Test case definitions
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

* **Peripheral**: Runs with ``-test=gap_peripheral`` and ``--peer_bd_address=${BD_ADDR[1]}`` to execute the ``gap_peripheral`` test suite
* **Central**: Runs with ``-test=gap_central`` and ``--peer_bd_address=${BD_ADDR[0]}`` to execute the ``gap_central`` test suite

The tests synchronize through Bluetooth operations (discovery, connection) rather than
direct inter-process communication.

Troubleshooting
***************

Port Allocation Failures
=========================

If tests fail with port allocation errors:

* Check for stale processes holding ports (example on Windows PowerShell):

  .. code-block:: console

     netstat -ano | Select-String ":<port>"

* Ensure cleanup handlers are properly releasing ports
* Verify no port conflicts with other services

Controller Startup Failures
============================

If simulated controllers fail to start:

* Verify Bumble version: ``pip show bumble`` (should be 0.0.220)
* Check Python dependencies:

  .. code-block:: console

     pip list

  (then confirm ``bumble==0.0.220`` is present)

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
* Check that the test registers the option using native_sim cmdline support
  (for example, see :zephyr_file:`tests/bluetooth/classic/sim/gap_discovery/src/test_main.c`)
* Review logs to confirm parameters are being received

Test Suite Selection Issues
============================

If the wrong test suite is running:

* Verify the ``-test`` parameter is correctly specified in the test script
* Check that test suite names match between the script and test code
* Ensure Ztest is properly configured to recognize the ``-test`` parameter
* Review test output to confirm which suite is executing
