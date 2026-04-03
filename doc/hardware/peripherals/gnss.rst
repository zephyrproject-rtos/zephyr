.. _gnss_api:

GNSS (Global Navigation Satellite System)
#########################################

Overview
********

GNSS is a general term which covers satellite systems used for
navigation, like GPS (Global Positioning System). GNSS services
are usually accessed through GNSS modems which receive and
process GNSS signals to determine their position, or more
specifically, their antennas position. They usually
additionally provide a precise time synchronization mechanism,
commonly named PPS (Pulse-Per-Second).

Subsystem support
*****************

The GNSS subsystem is based on the :ref:`modem`. The GNSS
subsystem covers everything from sending and receiving commands
to and from the modem, to parsing, creating and processing
NMEA0183 messages.

Adding support for additional NMEA0183 based GNSS modems
requires little more than implementing power management
and configuration for the specific GNSS modem.

Adding support for GNSS modems which use other protocols and/or
buses than the usual NMEA0183 over UART is possible, but will
require a bit more work from the driver developer.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_GNSS`
* :kconfig:option:`CONFIG_GNSS_SATELLITES`
* :kconfig:option:`CONFIG_GNSS_DUMP_TO_LOG`

Navigation Reference
********************

.. doxygengroup:: navigation

GNSS API Reference
******************

.. doxygengroup:: gnss_interface
