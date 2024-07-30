.. _timeutil_api:

Time Utilities
##############

Overview
********

:ref:`kernel_timing_uptime` in Zephyr is based on the a tick counter.  With
the default :kconfig:option:`CONFIG_TICKLESS_KERNEL` this counter advances at a
nominally constant rate from zero at the instant the system started. The POSIX
equivalent to this counter is something like ``CLOCK_MONOTONIC`` or, in Linux,
``CLOCK_MONOTONIC_RAW``.  :c:func:`k_uptime_get()` provides a millisecond
representation of this time.

Applications often need to correlate the Zephyr internal time with external
time scales used in daily life, such as local time or Coordinated Universal
Time.  These systems interpret time in different ways and may have
discontinuities due to `leap seconds <https://what-if.xkcd.com/26/>`__ and
local time offsets like daylight saving time.

Because of these discontinuities, as well as significant inaccuracies in the
clocks underlying the cycle counter, the offset between time estimated from
the Zephyr clock and the actual time in a "real" civil time scale is not
constant and can vary widely over the runtime of a Zephyr application.

The time utilities API supports:

* :ref:`converting between time representations <timeutil_repr>`
* :ref:`synchronizing and aligning time scales <timeutil_sync>`

For terminology and concepts that support these functions see
:ref:`timeutil_concepts`.

Time Utility APIs
*****************

.. _timeutil_repr:

Representation Transformation
=============================

Time scale instants can be represented in multiple ways including:

* Seconds since an epoch. POSIX representations of time in this form include
  ``time_t`` and ``struct timespec``, which are generally interpreted as a
  representation of `"UNIX Time"
  <https://tools.ietf.org/html/rfc8536#section-2>`__.

* Calendar time as a year, month, day, hour, minutes, and seconds relative to
  an epoch. POSIX representations of time in this form include ``struct tm``.

Keep in mind that these are simply time representations that must be
interpreted relative to a time scale which may be local time, UTC, or some
other continuous or discontinuous scale.

Some necessary transformations are available in standard C library
routines. For example, ``time_t`` measuring seconds since the POSIX EPOCH is
converted to ``struct tm`` representing calendar time with `gmtime()
<https://pubs.opengroup.org/onlinepubs/9699919799/functions/gmtime.html>`__.
Sub-second timestamps like ``struct timespec`` can also use this to produce
the calendar time representation and deal with sub-second offsets separately.

The inverse transformation is not standardized: APIs like ``mktime()`` expect
information about time zones.  Zephyr provides this transformation with
:c:func:`timeutil_timegm` and :c:func:`timeutil_timegm64`.

.. doxygengroup:: timeutil_repr_apis

.. _timeutil_sync:

Time Scale Synchronization
==========================

There are several factors that affect synchronizing time scales:

* The rate of discrete instant representation change.  For example Zephyr
  uptime is tracked in ticks which advance at events that nominally occur at
  :kconfig:option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` Hertz, while an external time
  source may provide data in whole or fractional seconds (e.g. microseconds).
* The absolute offset required to align the two scales at a single instant.
* The relative error between observable instants in each scale, required to
  align multiple instants consistently.  For example a reference clock that's
  conditioned by a 1-pulse-per-second GPS signal will be much more accurate
  than a Zephyr system clock driven by a RC oscillator with a +/- 250 ppm
  error.

Synchronization or alignment between time scales is done with a multi-step
process:

* An instant in a time scale is represented by an (unsigned) 64-bit integer,
  assumed to advance at a fixed nominal rate.
* :c:struct:`timeutil_sync_config` records the nominal rates of a reference
  time scale/source (e.g. TAI) and a local time source
  (e.g. :c:func:`k_uptime_ticks`).
* :c:struct:`timeutil_sync_instant` records the representation of a single
  instant in both the reference and local time scales.
* :c:struct:`timeutil_sync_state` provides storage for an initial instant, a
  recently received second observation, and a skew that can adjust for
  relative errors in the actual rate of each time scale.
* :c:func:`timeutil_sync_ref_from_local()` and
  :c:func:`timeutil_sync_local_from_ref()` convert instants in one time scale
  to another taking into account skew that can be estimated from the two
  instances stored in the state structure by
  :c:func:`timeutil_sync_estimate_skew`.

.. doxygengroup:: timeutil_sync_apis

.. _timeutil_concepts:

Concepts Underlying Time Support in Zephyr
******************************************

Terms from `ISO/TC 154/WG 5 N0038
<https://www.loc.gov/standards/datetime/iso-tc154-wg5_n0038_iso_wd_8601-1_2016-02-16.pdf>`__
(ISO/WD 8601-1) and elsewhere:

* A *time axis* is a representation of time as an ordered sequence of
  instants.
* A *time scale* is a way of representing an instant relative to an origin
  that serves as the epoch.
* A time scale is *monotonic* (increasing) if the representation of successive
  time instants never decreases in value.
* A time scale is *continuous* if the representation has no abrupt changes in
  value, e.g. jumping forward or back when going between successive instants.
* `Civil time <https://en.wikipedia.org/wiki/Civil_time>`__ generally refers
  to time scales that legally defined by civil authorities, like local
  governments, often to align local midnight to solar time.

Relevant Time Scales
====================

`International Atomic Time
<https://en.wikipedia.org/wiki/International_Atomic_Time>`__ (TAI) is a time
scale based on averaging clocks that count in SI seconds. TAI is a monotonic
and continuous time scale.

`Universal Time <https://en.wikipedia.org/wiki/Universal_Time>`__ (UT) is a
time scale based on Earth’s rotation. UT is a discontinuous time scale as it
requires occasional adjustments (`leap seconds
<https://en.wikipedia.org/wiki/Leap_second>`__) to maintain alignment to
changes in Earth’s rotation. Thus the difference between TAI and UT varies
over time. There are several variants of UT, with `UTC
<https://en.wikipedia.org/wiki/Coordinated_Universal_Time>`__ being the most
common.

UT times are independent of location. UT is the basis for Standard Time
(or "local time") which is the time at a particular location. Standard
time has a fixed offset from UT at any given instant, primarily
influenced by longitude, but the offset may be adjusted ("daylight
saving time") to align standard time to the local solar time. In a sense
local time is "more discontinuous" than UT.

`POSIX Time <https://tools.ietf.org/html/rfc8536#section-2>`__ is a time scale
that counts seconds since the "POSIX epoch" at 1970-01-01T00:00:00Z (i.e. the
start of 1970 UTC). `UNIX Time
<https://tools.ietf.org/html/rfc8536#section-2>`__ is an extension of POSIX
time using negative values to represent times before the POSIX epoch. Both of
these scales assume that every day has exactly 86400 seconds. In normal use
instants in these scales correspond to times in the UTC scale, so they inherit
the discontinuity.

The continuous analogue is `UNIX Leap Time
<https://tools.ietf.org/html/rfc8536#section-2>`__ which is UNIX time plus all
leap-second corrections added after the POSIX epoch (when TAI-UTC was 8 s).

Example of Time Scale Differences
---------------------------------

A positive leap second was introduced at the end of 2016, increasing the
difference between TAI and UTC from 36 seconds to 37 seconds. There was
no leap second introduced at the end of 1999, when the difference
between TAI and UTC was only 32 seconds. The following table shows
relevant civil and epoch times in several scales:

==================== ========== =================== ======= ==============
UTC Date             UNIX time  TAI Date            TAI-UTC UNIX Leap Time
==================== ========== =================== ======= ==============
1970-01-01T00:00:00Z 0          1970-01-01T00:00:08 +8      0
1999-12-31T23:59:28Z 946684768  2000-01-01T00:00:00 +32     946684792
1999-12-31T23:59:59Z 946684799  2000-01-01T00:00:31 +32     946684823
2000-01-01T00:00:00Z 946684800  2000-01-01T00:00:32 +32     946684824
2016-12-31T23:59:59Z 1483228799 2017-01-01T00:00:35 +36     1483228827
2016-12-31T23:59:60Z undefined  2017-01-01T00:00:36 +36     1483228828
2017-01-01T00:00:00Z 1483228800 2017-01-01T00:00:37 +37     1483228829
==================== ========== =================== ======= ==============

Functional Requirements
-----------------------

The Zephyr tick counter has no concept of leap seconds or standard time
offsets and is a continuous time scale. However it can be relatively
inaccurate, with drifts as much as three minutes per hour (assuming an RC
timer with 5% tolerance).

There are two stages required to support conversion between Zephyr time and
common human time scales:

* Translation between the continuous but inaccurate Zephyr time scale and an
  accurate external stable time scale;
* Translation between the stable time scale and the (possibly discontinuous)
  civil time scale.

The API around :c:func:`timeutil_sync_state_update()` supports the first step
of converting between continuous time scales.

The second step requires external information including schedules of leap
seconds and local time offset changes. This may be best provided by an
external library, and is not currently part of the time utility APIs.

Selecting an External Source and Time Scale
-------------------------------------------

If an application requires civil time accuracy within several seconds then UTC
could be used as the stable time source. However, if the external source
adjusts to a leap second there will be a discontinuity: the elapsed time
between two observations taken at 1 Hz is not equal to the numeric difference
between their timestamps.

For precise activities a continuous scale that is independent of local and
solar adjustments simplifies things considerably. Suitable continuous scales
include:

- GPS time: epoch of 1980-01-06T00:00:00Z, continuous following TAI with an
  offset of TAI-GPS=19 s.
- Bluetooth Mesh time: epoch of 2000-01-01T00:00:00Z, continuous following TAI
  with an offset of -32.
- UNIX Leap Time: epoch of 1970-01-01T00:00:00Z, continuous following TAI with
  an offset of -8.

Because C and Zephyr library functions support conversion between integral and
calendar time representations using the UNIX epoch, UNIX Leap Time is an ideal
choice for the external time scale.

The mechanism used to populate synchronization points is not relevant: it may
involve reading from a local high-precision RTC peripheral, exchanging packets
over a network using a protocol like NTP or PTP, or processing NMEA messages
received a GPS with or without a 1pps signal.
