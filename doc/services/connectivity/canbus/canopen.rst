.. _canopen:

CANopen Protocol Stack
######################

.. contents::
    :local:
    :depth: 2

Overview
********

CANopen is a higher-layer protocol for CAN, specified by CAN in Automation (CiA) in CiA 301. It
gives every device a node-ID, describes all of its data in an object dictionary, and defines how
the data is accessed (SDO), exchanged in real time (PDO), synchronized (SYNC), and how devices are
started, stopped and monitored (NMT, heartbeat, emergency messages).

The Zephyr CANopen stack is a native implementation on top of the :ref:`CAN controller API
<can_api>`. It is enabled with :kconfig:option:`CONFIG_CANOPEN`.

Features
********

* NMT slave with boot-up message, reset node and reset communication.
* Heartbeat producer (``1017h``) and consumer (``1016h``).
* SDO server with expedited and segmented transfers and the abort codes of CiA 301.
* Transmit and receive PDOs with the synchronous acyclic, synchronous cyclic and event-driven
  transmission types, inhibit time, event timer, SYNC start value and dynamic mapping.
* SYNC consumer and producer (``1005h``, ``1006h``, ``1007h``, ``1019h``).
* Emergency producer (``1014h``, ``1015h``), error register (``1001h``) and error history
  (``1003h``). The stack reports CAN overrun, error passive, recovery from bus-off, heartbeat
  timeout, RPDO length and timeout, and SYNC length errors.
* Error behavior (``1029h``) for bus-off and heartbeat timeouts.
* Node guarding and life guarding (``100Ch``, ``100Dh``), enabled with
  :kconfig:option:`CONFIG_CANOPEN_NODE_GUARDING`, which requires
  :kconfig:option:`CONFIG_CAN_ACCEPT_RTR`. Guarding requests are answered only while the producer
  heartbeat time (``1017h``) is 0. A life guarding timeout is reported with EMCY ``8130h`` and
  handled by the error behavior like a heartbeat timeout.

Not supported: the master side of node guarding, PDOs requested by remote frames, MPDO, the
CANopen manager boot-up of CiA 302, LSS (CiA 305) and CANopen FD (CiA 1301).

Object Dictionary
*****************

The stack provides the communication profile area (``1000h`` to ``1FFFh``). The application
describes its own entries with the ``CANOPEN_OD_*`` macros. The descriptors are constant, the
values are the application's own variables:

.. code-block:: c

   static uint32_t counter;
   static uint8_t output;

   CANOPEN_OD_DEFINE(app_od,
           CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_UNSIGNED32,
                          CANOPEN_OD_RO | CANOPEN_OD_ATTR_TPDO, counter),
           CANOPEN_OD_VAR(0x2001, CANOPEN_OD_TYPE_UNSIGNED8,
                          CANOPEN_OD_RW | CANOPEN_OD_ATTR_RPDO, output));

Entries are listed in ascending index order. Array and record entries are described with
:c:macro:`CANOPEN_OD_ARRAY` and :c:macro:`CANOPEN_OD_RECORD`. A write hook
(:c:macro:`CANOPEN_OD_VAR_HOOK`) can validate or reject values written over the network.

The application reads and writes values with :c:func:`canopen_od_read` and
:c:func:`canopen_od_write`, which take the node lock.

Node configuration
******************

A node is described by a :c:struct:`canopen_node_config` with the CAN controller, the
node-ID, the object dictionary, the power-on configuration of the PDOs
(:c:struct:`canopen_pdo_config`), the heartbeat consumer entries and the
callbacks. It is started with :c:func:`canopen_node_init` and :c:func:`canopen_node_start`.
The bitrate is set by the application with :c:func:`can_set_bitrate` before the node is
started.

The node runs in its own thread. The CAN RX callback only queues frames. The thread processes
the frames and the protocol timers and calls the application callbacks.

CAN controller resources
************************

The stack installs one CAN RX filter for each CAN-ID it receives: NMT, SDO, SYNC, each valid
RPDO and each configured heartbeat consumer entry. Applications must not install their own
filters for these CAN-IDs: on many controllers a received frame is passed to one matching filter
only.

Bus state errors are taken from :c:func:`can_add_state_change_callback`. Bus-off recovery uses
the CAN controller's automatic recovery. With :kconfig:option:`CONFIG_CAN_MANUAL_RECOVERY_MODE`
the application calls :c:func:`can_recover`.

Samples
*******

* :zephyr:code-sample:`canopen`

API Reference
*************

.. doxygengroup:: canopen
