.. _bluetooth_classic_l2cap_echo_test:

Bluetooth Classic L2CAP Echo Test
#################################

Overview
********

This test suite verifies the Bluetooth Classic (BR/EDR) L2CAP Echo Request/Response
(also known as the *Echo* signaling command) behavior using the Bumble Bluetooth
controller simulator sidecar.

The test is executed on the Zephyr ``native_sim`` platform and uses the
`Bumble <https://github.com/google/bumble>`_ Bluetooth controller simulator.
Two simulated controllers are started by Twister's Bumble sidecar and connected
to two instances of the same test binary via ``--bt-dev=127.0.0.1:<tcp_port>``.
Each instance receives a ``--peer_bd_address`` argument so the two roles can
locate each other.

The test binary is started twice:

* **Device 1 role** (``l2cap_echo_device1::...`` ztest suite): performs BR/EDR
  inquiry, initiates a BR/EDR ACL connection to the peer, sends L2CAP Echo
  Requests, and validates received Echo Responses. It also receives L2CAP Echo
  Requests from Device 2 and sends Echo Responses back.
* **Device 2 role** (``l2cap_echo_device2::...`` ztest suite): waits for an
  incoming BR/EDR ACL connection, receives L2CAP Echo Requests, and sends Echo
  Responses back. It also sends L2CAP Echo Requests to Device 1 and validates
  received Echo Responses.

Implementation Notes
********************

* The peer BD_ADDR is passed on the command line and parsed by a
  :c:macro:`NATIVE_TASK` registration in :file:`src/test_main.c`.
* Both roles register the Echo callbacks via
  :c:func:`bt_l2cap_br_echo_cb_register`.
* The test uses a minimal fixed-size net_buf pool for signaling payloads.
* Device discovery uses BR/EDR inquiry (GAP discovery) and matches the
  configured peer address before attempting :c:func:`bt_conn_create_br`.

Test Cases
**********

``bluetooth.classic.sim.l2cap_echo.basic``
  - **Device 1** (``l2cap_echo_device1::test_01_send_echo_req_and_recv_rsp``):
    Sends Echo Requests to Device 2 and validates that Echo Responses are
    received correctly; also handles Echo Requests sent by Device 2.
  - **Device 2** (``l2cap_echo_device2::test_01_send_echo_req_and_recv_rsp``):
    Handles Echo Requests from Device 1; also sends Echo Requests to Device 1
    and validates the responses.

The scenario validates:

* BR/EDR inquiry and peer matching by BD_ADDR
* BR/EDR connection establishment and teardown
* L2CAP Echo Request -> Echo Response flow (both directions)
* Handling of outstanding Echo transactions
* Input validation for Echo Response (identifier ``0`` is rejected with
  ``-EINVAL``)

Requirements
************

* Host environment capable of running ``native_sim`` tests.
* Python package ``bumble`` installed (used by Twister's Bumble sidecar).

Building and Running
********************

This test is intended to be run via Twister on ``native_sim``.

.. code-block:: console

   west twister -T tests/bluetooth/classic/bumble/l2cap_echo

Twister invokes the ``bumble`` sidecar defined in :file:`tests.yaml`. The
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
   INFO    - Total complete:    1/   1  100%  built (not run):    0, filtered:    0, failed:    0, error:    0
   INFO    - 1 test scenarios (1 configurations) selected, 0 configurations filtered (0 by static filter, 0 at runtime).
   INFO    - 1 of 1 executed test configurations passed (100.00%), 0 built (not run), 0 failed, 0 errored, with no warnings in XX.XX seconds.
   INFO    - Saving reports...

Test Coverage
*************

This test covers:

* Bluetooth Classic initialization and de-initialization
* BR/EDR inquiry and ACL connection establishment
* L2CAP BR/EDR Echo signaling commands (Echo Request and Echo Response)
* Bidirectional Echo Request/Response exchange
* Echo callback registration (``bt_l2cap_br_echo_cb_register``)
* Basic error handling (invalid Echo Response identifier)

Configuration Options
*********************

See :file:`prj.conf` for the default configuration.

Additional configuration options can be found in :file:`Kconfig`.
