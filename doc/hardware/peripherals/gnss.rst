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

Power management
*****************

GNSS receivers typically begin acquiring and tracking GNSS signals as
soon as they are powered on, unless configured otherwise. In order to
preserve power, an application can either remove power from the GNSS
receiver or stop the internal GNSS engine from actively acquiring and
tracking signals. The latter option halts position computation while
keeping the receiver powered with reduced power consumption and the
communication link to the host application available for other operations
or for resuming the GNSS engine.

In the GNSS subsystem, this can be done using the :c:func:`gnss_stop`
API call. This is distinct from Zephyr's device power management
(suspend/resume): the receiver remains powered and able to communicate
with the host application, but GNSS tracking is stopped.

GNSS tracking can be resumed using :c:func:`gnss_start`. The application
can specify a :c:enum:`gnss_start_mode`, which affects time-to-first-fix.
A hot start preserves navigation data and allows the receiver to reacquire
signals quickly, while warm and cold starts discard some or all navigation
data, respectively, resulting in longer acquisition times.

GNSS device drivers must ensure the GNSS modem starts tracking when
resumed from a suspended or powered-off state, without requiring the
application to call :c:func:`gnss_start` explicitly. Most receivers will
end up performing a cold start in this situation, simply because they
lost their tracking data while unpowered, but a driver may instead
attempt a warm or hot start if the modem retained enough state to do so
faster. The requirement is that tracking resumes automatically on power
up; clearing navigation data is only a side effect of the modem having
been unpowered, not an intended part of resuming from suspend.

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
