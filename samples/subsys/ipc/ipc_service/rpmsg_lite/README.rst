.. zephyr:code-sample:: rpmsg-lite
   :name: IPC service: RPMSG-Lite backend
   :relevant-api: ipc

   Send messages between two cores using the IPC service and RPMSG-Lite backend.

Overview
********

This application demonstrates how to use the IPC Service with the RPMSG-Lite
backend in Zephyr. It is designed to show how to integrate the backend from
both a build and code perspective.

The sample is functionally equivalent to the OpenAMP ``static_vrings`` IPC
service sample but uses RPMSG-Lite as the underlying transport.

Three IPC instances are demonstrated, each running sequentially (instance N
starts only after instance N-1 has completed its exchange):

- **Instance 0**: standard copy-based send/receive between host and remote
  using a single named endpoint.
- **Instance 1**: zero-copy nocopy API (``ipc_service_get_tx_buffer`` /
  ``ipc_service_send_nocopy`` / ``ipc_service_release_rx_buffer``).
- **Instance 2**: second copy-based instance, demonstrating that the
  ``platform_notify()`` owner-search correctly routes notifications to the
  matching link-id even when multiple non-owner instances share a single
  physical MBOX channel.

RPMSG-Lite requires an MBOX driver with data-transfer mode. A single pair
of MBOX channels (TX + RX) is shared across all IPC instances via the
``notification-parent`` DT property; the ``link-id`` DT property selects
which RPMSG-Lite link a given instance belongs to. Link IDs must be
sequential starting from 0.

Building and Running
********************

The sample supports the boards listed in ``tests.yaml``. Build with sysbuild
to compile both the host and remote images in one step:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipc_service/rpmsg_lite
   :board: mimxrt1170_evk@B/mimxrt1176/cm7
   :goals: build flash
   :west-args: --sysbuild

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following messages will appear on the corresponding
serial ports (one for host, one for remote):

.. code-block:: console

   *** Booting Zephyr OS build v4.x.x ***
   IPC-service HOST [INST 0] demo started
   HOST [0]: 1
   HOST [0]: 3
   HOST [0]: 5
   HOST [0]: 7
   HOST [0]: 9
   HOST [0]: 11
   HOST [0]: 13
   HOST [0]: 15
   HOST [0]: 17
   HOST [0]: 19
   IPC-service HOST [INST 0] demo ended.
   IPC-service HOST [INST 1] demo started
   HOST [1]: 1
   HOST [1]: 3
   HOST [1]: 5
   ...
   HOST [1]: 37
   HOST [1]: 39
   IPC-service HOST [INST 1] demo ended.
   IPC-service HOST [INST 2] demo started
   HOST [2]: 1
   HOST [2]: 3
   HOST [2]: 5
   HOST [2]: 7
   HOST [2]: 9
   HOST [2]: 11
   HOST [2]: 13
   HOST [2]: 15
   HOST [2]: 17
   HOST [2]: 19
   IPC-service HOST [INST 2] demo ended.

.. code-block:: console

   *** Booting Zephyr OS build v4.x.x ***
   IPC-service REMOTE [INST 0] demo started
   REMOTE [0]: 0
   REMOTE [0]: 2
   REMOTE [0]: 4
   REMOTE [0]: 6
   REMOTE [0]: 8
   REMOTE [0]: 10
   REMOTE [0]: 12
   REMOTE [0]: 14
   REMOTE [0]: 16
   REMOTE [0]: 18
   IPC-service REMOTE [INST 0] demo ended.
   IPC-service REMOTE [INST 1] demo started
   REMOTE [1]: 0
   REMOTE [1]: 2
   REMOTE [1]: 4
   ...
   REMOTE [1]: 36
   REMOTE [1]: 38
   IPC-service REMOTE [INST 1] demo ended.
   IPC-service REMOTE [INST 2] demo started
   REMOTE [2]: 0
   REMOTE [2]: 2
   REMOTE [2]: 4
   REMOTE [2]: 6
   REMOTE [2]: 8
   REMOTE [2]: 10
   REMOTE [2]: 12
   REMOTE [2]: 14
   REMOTE [2]: 16
   REMOTE [2]: 18
   IPC-service REMOTE [INST 2] demo ended.
