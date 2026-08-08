.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
.. SPDX-License-Identifier: Apache-2.0

.. _precision_timing:

Precision Timing
################

The experimental precision timing subsystem provides protocol-neutral helpers
for applications and subsystems that need to relate multiple clock domains.
It is enabled by :kconfig:option:`CONFIG_PRECISION_TIMING`.

The subsystem is intentionally below protocol layers such as PTP and gPTP. It
does not define grandmaster selection, clock quality, packet formats, or network
state machines. Protocols remain responsible for source selection and for
deciding when source changes invalidate synchronization state.

Clock Domains
*************

Every :c:struct:`precision_time_point` carries a
:c:struct:`precision_time_domain`. Raw counter values are not assumed to be TAI,
UTC, monotonic time, or PHC time unless the caller labels them with a domain.
Operations that combine or compare time points reject mismatched domains.

Domain mappings are represented by :c:struct:`precision_time_mapping`. The
mapping helper uses :c:struct:`timeutil_sync_state` internally and converts
between source and local domains with explicit ``-EAGAIN`` errors while no valid
mapping is available. A hard clock step, synchronization source change, or
discipline fault should invalidate the mapping before new observations are
accepted.

Clock Abstraction
*****************

:c:struct:`precision_clock` describes a clock with optional operations for read,
set, phase adjustment, rate adjustment, and capability queries. Rate adjustment
uses signed parts-per-billion at the precision timing boundary.

The PTP clock adapter exposes existing PTP hardware clocks through this API. It
infers conservative capabilities from legacy callbacks, and uses driver-reported
limits when the PTP clock driver implements the optional capability callback.
This keeps existing :c:func:`ptp_clock_get`, :c:func:`ptp_clock_set`,
:c:func:`ptp_clock_adjust`, and :c:func:`ptp_clock_rate_adjust` users working
while allowing precision timing users to query resolution, phase-adjustment
limits, and rate range.

Discipline Engine
*****************

:c:struct:`precision_pi_discipline` is an instance-based PI discipline. It
accepts domain-qualified observations and returns a control decision instead of
touching hardware directly:

* ``PRECISION_DISCIPLINE_STEP`` for large phase corrections.
* ``PRECISION_DISCIPLINE_ADJUST_RATE`` for normal frequency discipline.
* ``PRECISION_DISCIPLINE_IGNORE`` for rejected or stale samples.
* ``PRECISION_DISCIPLINE_RESET`` when outlier policy requires reacquisition.

The engine tracks ``UNSYNCED``, ``ACQUIRING``, ``LOCKED``, ``HOLDOVER``, and
``FAULT`` style state without embedding PTP-specific clock quality fields.
Source timeout handling enters holdover first, then resets the discipline when
the configured holdover interval expires. A clock-operation failure enters the
sticky ``FAULT`` state and blocks further control until the protocol explicitly
resets the discipline.

Protocol Integration
********************

PTP and gPTP use the shared PI discipline and the PTP-clock precision adapter
for PHC control. Their protocol datasets, packet timestamp storage, management
messages, state machines, and application APIs remain unchanged.

When a protocol performs a hard clock step, loses its synchronization source, or
changes source, it clears protocol delay samples and invalidates the associated
domain mapping. Normal accepted synchronization observations update both the PI
discipline and the source-to-local mapping.

Relation To The PTP Roadmap
***************************

This subsystem is stage 2 of the PTP/gPTP roadmap discussed in
`issue 107837 <https://github.com/zephyrproject-rtos/zephyr/issues/107837>`_,
which lists conversion between explicitly identified time domains as one of the
reusable components to provide.

The domain mapping is therefore maintained by PTP and gPTP even though those
protocols only need the discipline today. The stage 3 clock synchronization
service disciplines one adjustable clock from another, for example a PHC from
another PHC or an application-visible clock from a PHC, and consumes the
mapping to express one domain in terms of another. Keeping the mapping
alongside the discipline means that a source change, hard step, or fault
invalidates both in one place instead of requiring the later service to
reconstruct history.

API Reference
*************

.. doxygengroup:: precision_timing

.. doxygengroup:: precision_ptp_clock
