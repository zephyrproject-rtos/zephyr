.. _ptp_interface:

Precision Time Protocol (PTP)
#############################

.. contents::
    :local:
    :depth: 2

Overview
********

PTP is a network protocol implemented in the application layer, used to synchronize
clocks in a computer network. It's accurate up to less than a microsecond.
The stack supports the protocol and procedures as defined in the `IEEE 1588-2019 standard`_
(IEEE Standard for a Precision Clock Synchronization Protocol
for Networked Measurement and Control Systems). It has multiple profiles,
and can be implemented on top of L2 (Ethernet) or L3 (UDP/IPv4 or UDP/IPv6).
Its accuracy is achieved by using hardware timestamping of the protocol packets.

Zephyr's implementation of PTP stack consist following items:

* PTP stack thread that handles incoming messages and events
* Integration with ptp_clock driver
* PTP stack initialization executed during system init

The implementation automatically creates PTP Ports (each PTP Port corresponds to unique interface).

Supported features
******************

Implementation of the stack doesn't support all features specified in the standard.
In the table below all supported features are listed.

.. csv-table:: Supported features
   :header: Feature, Supported
   :widths: 50,10

    Ordinary Clock, yes
    Boundary Clock, yes
    Transparent Clock,
    Management Node,
    End to end delay mechanism, yes
    Peer to peer delay mechanism,
    Multicast operation mode,
    Hybrid operation mode,
    Unicast operation mode,
    Non-volatile storage,
    UDP IPv4 transport protocol, yes
    UDP IPv6 transport protocol, yes
    IEEE 802.3 (Ethernet) transport protocol, yes
    Hardware timestamping, yes
    Software timestamping,
    TIME_RECEIVER_ONLY PTP Instance, yes
    TIME_TRANSMITTER_ONLY PTP Instance,

Supported Management messages
*****************************

Based on Table 59 from section 15.5.2.3 of the IEEE 1588-2019 following management TLVs
are supported:

.. csv-table:: Supported management message's IDs
   :header: Management_ID, Management_ID name, Allowed actions
   :widths: 10,40,25

    0x0000, NULL_PTP_MANAGEMENT, GET SET COMMAND
    0x0001, CLOCK_DESCRIPTION, GET
    0x0002, USER_DESCRIPTION, GET
    0x0003, SAVE_IN_NON_VOLATILE_STORAGE, -
    0x0004, RESET_NON_VOLATILE_STORAGE, -
    0x0005, INITIALIZE, -
    0x0006, FAULT_LOG, -
    0x0007, FAULT_LOG_RESET, -
    0x2000, DEFAULT_DATA_SET, GET
    0x2001, CURRENT_DATA_SET, GET
    0x2002, PARENT_DATA_SET, GET
    0x2003, TIME_PROPERTIES_DATA_SET, GET
    0x2004, PORT_DATA_SET, GET
    0x2005, PRIORITY1, GET SET
    0x2006, PRIORITY2, GET SET
    0x2007, DOMAIN, GET SET
    0x2008, TIME_RECEIVER_ONLY, GET SET
    0x2009, LOG_ANNOUNCE_INTERVAL, GET SET
    0x200A, ANNOUNCE_RECEIPT_TIMEOUT, GET SET
    0x200B, LOG_SYNC_INTERVAL, GET SET
    0x200C, VERSION_NUMBER, GET SET
    0x200D, ENABLE_PORT, COMMAND
    0x200E, DISABLE_PORT, COMMAND
    0x200F, TIME, GET SET
    0x2010, CLOCK_ACCURACY, GET SET
    0x2011, UTC_PROPERTIES, GET SET
    0x2012, TRACEBILITY_PROPERTIES, GET SET
    0x2013, TIMESCALE_PROPERTIES, GET SET
    0x2014, UNICAST_NEGOTIATION_ENABLE, -
    0x2015, PATH_TRACE_LIST, -
    0x2016, PATH_TRACE_ENABLE, -
    0x2017, GRANDMASTER_CLUSTER_TABLE, -
    0x2018, UNICAST_TIME_TRANSMITTER_TABLE, -
    0x2019, UNICAST_TIME_TRANSMITTER_MAX_TABLE_SIZE, -
    0x201A, ACCEPTABLE_TIME_TRANSMITTER_TABLE, -
    0x201B, ACCEPTABLE_TIME_TRANSMITTER_TABLE_ENABLED, -
    0x201C, ACCEPTABLE_TIME_TRANSMITTER_MAX_TABLE_SIZE, -
    0x201D, ALTERNATE_TIME_TRANSMITTER, -
    0x201E, ALTERNATE_TIME_OFFSET_ENABLE, -
    0x201F, ALTERNATE_TIME_OFFSET_NAME, -
    0x2020, ALTERNATE_TIME_OFFSET_MAX_KEY, -
    0x2021, ALTERNATE_TIME_OFFSET_PROPERTIES, -
    0x3000, EXTERNAL_PORT_CONFIGURATION_ENABLED,
    0x3001, TIME_TRANSMITTER_ONLY, -
    0x3002, HOLDOVER_UPGRADE_ENABLE, -
    0x3003, EXT_PORT_CONFIG_PORT_DATA_SET, -
    0x4000, TRANSPARENT_CLOCK_DEFAULT_DATA_SET, -
    0x4001, TRANSPARENT_CLOCK_PORT_DATA_SET, -
    0x4002, PRIMARY_DOMAIN, -
    0x6000, DELAY_MECHANISM, GET
    0x6001, LOG_MIN_PDELAY_REQ_INTERVAL, GET SET

Timestamping notes
******************

When RX hardware timestamps are unavailable or invalid, synchronization falls
back to reading PHC time during receive processing. This can introduce
additional jitter from software handling latency (packet path and scheduling)
between frame arrival and PHC read.

This behavior is expected for L2 AF_PACKET paths without true driver-provided
RX hardware timestamps.

For IEEE 802.3 transport, Sync is sent in two-step mode and Follow_Up is
generated from TX timestamp callbacks. If a TX timestamp is missing or late,
the stack logs a warning and skips Follow_Up for that Sync sequence, then
continues normal Sync transmission on subsequent intervals (best-effort
behavior).

Supported hardware
******************

Although the stack itself is hardware independent, Ethernet frame timestamping
support must be enabled in ethernet drivers.

Boards supported:

- :zephyr:board:`nucleo_h563zi`
- :zephyr:board:`nucleo_h743zi`
- :zephyr:board:`nucleo_h745zi_q`
- :zephyr:board:`nucleo_f767zi`
- :zephyr:board:`native_sim` (only usable for simple testing, limited capabilities
  due to lack of hardware clock)

Enabling the stack
******************

The following configuration option must me enabled in :file:`prj.conf` file.

- :kconfig:option:`CONFIG_PTP`

Testing
*******

The stack has been informally tested using the
`Linux ptp4l <https://linuxptp.sourceforge.net/>`_ daemons.
It has also been tested with the :zephyr:board:`nucleo_h563zi` board against a GPS clock,
both with a direct Ethernet connection and with a PTP-capable switch in between.
All tests were performed using the :zephyr:code-sample:`PTP sample application <ptp>`,
with UDP IPv4, UDP IPv6, and IEEE 802.3 transport.

The following table summarizes the informal test matrix:

+--------------------------------+-------------+----------+----------+
|                                | IEEE 802.3  | UDP IPv4 | UDP IPv6 |
+================================+=============+==========+==========+
| ptp4l daemons                  | yes         | yes      | yes      |
+--------------------------------+-------------+----------+----------+
| GPS Clock (direct link)        | yes         | yes      | yes      |
+--------------------------------+-------------+----------+----------+
| PTP-capable switch             | yes         | yes      | yes      |
+--------------------------------+-------------+----------+----------+
| GPS Clock + PTP-capable switch | yes         | yes      | yes      |
+--------------------------------+-------------+----------+----------+

For all tested configurations the device was configured as an Ordinary Clock.
In theory, the stack should also be able to operate as a Boundary Clock,
but this mode of operation has not been validated due to lack of dev-boards
with multiple Ethernet interfaces on the market.

The :zephyr:code-sample:`PTP sample application <ptp>` from the Zephyr
source distribution can be used for testing.

.. _IEEE 1588-2019 standard:
   https://standards.ieee.org/ieee/1588/6825/

API Reference
*************

.. doxygengroup:: ptp
