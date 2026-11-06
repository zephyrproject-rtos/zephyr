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
* DOMAIN objects (:c:macro:`CANOPEN_OD_DOMAIN`), enabled with
  :kconfig:option:`CONFIG_CANOPEN_DOMAIN`: data of any size, streamed by the SDO server through the
  read and write callbacks of a :c:struct:`canopen_od_domain`.
* SDO block upload and block download with CRC, enabled with
  :kconfig:option:`CONFIG_CANOPEN_SDO_BLOCK`. Lost segments are repeated as acknowledged by the
  client or server. The server always answers a block upload with block transfer and does not use
  the protocol switch threshold. The block size of a download is
  :kconfig:option:`CONFIG_CANOPEN_SDO_BLOCK_SIZE`.
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
* A ``canopen`` shell command, enabled with :kconfig:option:`CONFIG_CANOPEN_SHELL`: list the local
  nodes, change their NMT state, read and write their objects and show the error history, and
  with the master functions read and write other nodes and send NMT commands.
* TIME producer and consumer (``1012h``), enabled with :kconfig:option:`CONFIG_CANOPEN_TIME`.
  Received time stamps are passed to the ``time_cb`` callback, :c:func:`canopen_time_send`
  sends one.
* Store and restore parameters (``1010h``, ``1011h``) through the :ref:`settings subsystem
  <settings_api>`, enabled with :kconfig:option:`CONFIG_CANOPEN_STORAGE`.
* RUN and ERR indicator LEDs according to CiA 303-3, enabled with
  :kconfig:option:`CONFIG_CANOPEN_LEDS`.
* Master functions, enabled with :kconfig:option:`CONFIG_CANOPEN_MASTER`: an SDO client
  (``1280h``) with expedited and segmented transfers, NMT node control commands and a simple
  boot-up of slaves.

Not supported: SDO block transfer, the master side of node guarding, PDOs requested by remote
frames, MPDO, the CANopen manager boot-up of CiA 302, LSS (CiA 305) and CANopen FD (CiA 1301).

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
Entries marked with :c:macro:`CANOPEN_OD_ATTR_PERSIST` are stored with ``1010h``.

The application reads and writes values with :c:func:`canopen_od_read` and
:c:func:`canopen_od_write`, which take the node lock.

Electronic data sheet
*********************

An electronic data sheet (EDS, CiA 306) describes the object dictionary of a node to PC tools.
Two tools in :zephyr_file:`scripts/canopen` connect it to the stack, in both directions.

With :kconfig:option:`CONFIG_CANOPEN_EDS` the build writes ``zephyr/canopen.eds`` from the linked
image. The node configuration is named with :kconfig:option:`CONFIG_CANOPEN_EDS_NODE_CONFIG`. The
application objects, the default values of their variables, the PDO and heartbeat consumer
configuration and the serial number are read from the image, the communication objects follow
from the Kconfig options. :kconfig:option:`CONFIG_CANOPEN_EDS_VENDOR_NAME` and
:kconfig:option:`CONFIG_CANOPEN_EDS_BITRATES` fill in ``VendorName`` and the supported bit rates.

The CMake function ``canopen_od_from_eds()`` generates the object dictionary from an EDS or DCF
instead:

.. code-block:: cmake

   canopen_od_from_eds(app ${CMAKE_CURRENT_SOURCE_DIR}/node.eds node_od)

The generated header ``node_od.h`` defines the variables, ``node_od`` and the PDO and heartbeat
consumer configuration as macros for the node configuration:

.. code-block:: c

   #include "node_od.h"

   static const struct canopen_node_config node_cfg = {
           .od = &node_od,
           .tpdo = NODE_OD_TPDO,
           .tpdo_count = NODE_OD_TPDO_COUNT,
           .rpdo = NODE_OD_RPDO,
           .rpdo_count = NODE_OD_RPDO_COUNT,
           .hb_consumer = NODE_OD_HB_CONSUMER,
           .hb_consumer_count = NODE_OD_HB_CONSUMER_COUNT,
           .serial_number = NODE_OD_SERIAL_NUMBER,
           ...
   };

The communication objects of the EDS must match the objects the stack provides with the
Kconfig options of the build, and the values that follow from Kconfig must be equal, otherwise
the build fails with a list of the differences. The EDS has no field for the length of a string,
so a string object gets the length of its default value. Objects stored with ``1010h`` are
marked with ``StorageLocation=PERSIST``, an entry that is not defined by CiA 306.

Node configuration
******************

A node is described by a :c:struct:`canopen_node_config` with the CAN controller, the
node-ID, the object dictionary, the power-on configuration of the PDOs
(:c:struct:`canopen_pdo_config`), the heartbeat consumer entries, the optional LEDs and the
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

Master functions
****************

:c:func:`canopen_sdo_upload` and :c:func:`canopen_sdo_download` read and write the object
dictionary of another node. The transfer runs in the CANopen thread while the calling thread
waits, so they must not be called from a node callback. A server node-ID of 1 to 127 selects the
default SDO of that node, 0 uses the SDO client parameter as configured.

:c:func:`canopen_nmt_send` sends an NMT command to one node or to all nodes. The NMT state of
the sending node itself is changed with :c:func:`canopen_nmt_local_cmd`.

:c:func:`canopen_master_boot` brings up a list of slaves: it waits for each slave to answer an
SDO read of the device type, writes its configuration values and starts it. The remote NMT
states are monitored with the heartbeat consumer and :c:func:`canopen_hb_consumer_state`.

Indicator LEDs
**************

The RUN and ERR LEDs are given as :c:struct:`led_dt_spec` in the node configuration, for
example from ``gpio-leds`` nodes with :c:macro:`LED_DT_SPEC_GET`. The RUN LED blinks in
pre-operational, single flashes in stopped and is on in operational. The ERR LED is on at
bus-off, double flashes after a heartbeat consumer timeout, triple flashes when no SYNC is
received within 1.5 times the communication cycle period (``1006h``), quadruple flashes after
an RPDO timeout and single flashes while the CAN controller is in error warning or error
passive state. With ``led_bicolor`` the two LEDs are the colors of one status LED and red has
priority.

Samples
*******

* :zephyr:code-sample:`canopen`

API Reference
*************

.. doxygengroup:: canopen
