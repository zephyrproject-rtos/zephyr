.. _bluetooth_classic_l2cap_echo_test:

Bluetooth Classic L2CAP Echo Test
#################################

Overview
********

This test suite verifies the Bluetooth Classic (BR/EDR) L2CAP Echo Request/Response
(also known as the *Echo* signaling command) behavior using the Bluetooth Classic
simulation test infrastructure.

The test is executed on the Zephyr ``native_sim`` platform and uses the
`Bumble <https://github.com/google/bumble>`_ Bluetooth controller simulator.
Two simulated controllers are started and connected to two instances of the same
test binary via ``--bt-dev=127.0.0.1:<tcp_port>``.

The test binary is started twice:

* **Device 1 role**: runs the ``l2cap_echo_device1::...`` ztest suite. It performs
  BR/EDR inquiry, initiates a BR/EDR ACL connection to the peer, sends L2CAP Echo
  Requests, and validates received Echo Responses. Also, it receives L2CAP Echo
  Requests, and sends Echo Responses back.
* **Device 2 role**: runs the ``l2cap_echo_device2::...`` ztest suite. It waits for
  an incoming BR/EDR ACL connection, receives L2CAP Echo Requests, and sends Echo
  Responses back. Also, it sends L2CAP Echo Requests, and validates received Echo
  Responses.

Implementation notes
********************

* The peer BD_ADDR is passed on the command line and parsed by
  :c:macro:`NATIVE_TASK` registration in :file:`src/test_main.c`.
* Both roles register the Echo callbacks via
  :c:func:`bt_l2cap_br_echo_cb_register`.
* The test uses a minimal fixed-size net_buf pool for signaling payloads.
* Device discovery uses BR/EDR inquiry (GAP discovery) and matches the configured
  peer address before attempting :c:func:`bt_conn_create_br`.

Test Scenarios
**************

Scenarios are defined in :file:`testcase.yaml` and implemented by scripts in
:file:`tests_scripts/`.

The scripts follow the common Bluetooth Classic simulation harness pattern:

* allocate two free TCP ports (HCI transport),
* start two Bumble controllers, each bound to one TCP port and one fixed BD_ADDR,
* run the same ``zephyr.exe`` twice, once as Device 1 and once as Device 2.

Available scenarios:

* ``bluetooth.classic.sim.l2cap_echo.basic``

  * Script: :file:`tests_scripts/basic.sh`
  * Device 1 ztest: ``-test=l2cap_echo_device1::test_01_send_echo_req_and_recv_rsp``
  * Device 2 ztest: ``-test=l2cap_echo_device2::test_01_send_echo_req_and_recv_rsp``

What is tested
**************

The basic scenario validates:

* BR/EDR inquiry and peer matching by BD_ADDR
* BR/EDR connection establishment and teardown
* L2CAP Echo Request -> Echo Response flow
* Handling of outstanding Echo transactions (second request does not immediately
  get a response until the peer responds)
* Input validation for Echo Response (identifier ``0`` is rejected with ``-EINVAL``)

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

   west twister -T tests/bluetooth/classic/sim/l2cap_echo

Direct script execution
=======================

Twister uses the script harness defined in :file:`testcase.yaml` and runs the
scripts listed in ``harness_config/tests_scripts``.

For example:

.. code-block:: console

   ./tests/bluetooth/classic/sim/l2cap_echo/tests_scripts/basic.sh

Each script typically:

* allocates two TCP ports,
* starts two Bumble controllers (one per port, with fixed BD_ADDR values),
* launches the test binary twice:

  * Device 1 instance (e.g. ``-test=l2cap_echo_device1::...``)
  * Device 2 instance (e.g. ``-test=l2cap_echo_device2::...``)

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
   INFO    - 1 of 1 executed test configurations passed (100.00%), 0 built (not run), 0 failed, 0 errored, with no warnings in XX.XX seconds.
   INFO    - Saving reports...

Test Coverage
*************

This test covers:

* Bluetooth Classic initialization
* BR/EDR inquiry and ACL connection establishment
* L2CAP BR/EDR Echo signaling commands (Echo Req/Rsp)
* Echo callback registration and basic error handling

Configuration Options
*********************

See :file:`prj.conf` for the default configuration.

Additional configuration options can be found in :file:`Kconfig`.
