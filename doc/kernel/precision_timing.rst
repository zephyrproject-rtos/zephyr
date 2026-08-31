.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
.. SPDX-License-Identifier: Apache-2.0

.. _precision_timing:

Precision Timing
################

The precision timing subsystem provides a small set of reusable mechanisms for
PTP, gPTP, and other users that control high-resolution clocks. Enable it with
:kconfig:option:`CONFIG_PRECISION_TIMING`.

.. note::

   The API is at :ref:`experimental <api_lifecycle_experimental>` maturity and
   may still change. A protocol does not change maturity by using this
   subsystem.

The subsystem deliberately does not implement synchronization policy. PTP and
gPTP continue to own their protocol state machines, clock selection, step
thresholds, sample acceptance, lock detection, source-loss handling, and
diagnostics.

Precision time
**************

:c:type:`precision_time_t` is a signed 64-bit nanosecond value. Checked
addition and subtraction helpers report overflow. The type does not identify a
time domain or timescale; modelling TAI, UTC, PHCs, monotonic time, and protocol
relationships is outside this API.

Precision clock
***************

:c:struct:`precision_clock` dispatches four mandatory clock operations:

* read the current time;
* set an absolute time;
* apply a phase adjustment; and
* set a rate offset from the nominal frequency as parts per million with a
  16-bit binary fractional field.

All operations must be implemented by a clock adapter. Adjustment ranges and
other hardware constraints remain the responsibility of the underlying clock
implementation.

:c:struct:`precision_clock_ptp_adapter` exposes an existing Zephyr PTP clock
device through this interface. It performs the required conversion between
:c:struct:`net_ptp_time` and :c:type:`precision_time_t`; it does not add
capability discovery or synchronization state.

PI controller
*************

:c:struct:`precision_pi` is an instance-based proportional-integral controller.
Each instance stores its own gains and integral term. For every error sample,
the update is equivalent to:

.. code-block:: c

   integral += ki * error;
   output = kp * error + integral;

The controller does not decide whether an error should be stepped, rejected, or
used for rate adjustment. It also does not track synchronization, acquisition,
lock, holdover, source timeout, or clock faults. Callers own those decisions and
reset the accumulated integral term when their policy requires it.

Protocol integration
********************

PTP and the gPTP default clock-update path each keep their existing policy and
use a :c:struct:`precision_pi` for the shared calculation. They initialize a
PTP-clock adapter once and use :c:struct:`precision_clock` operations to access
the PHC.

Sample
******

The :zephyr:code-sample:`precision_timing` sample demonstrates checked time
arithmetic, a software-backed precision clock, and PI-driven rate adjustment.

API reference
*************

.. doxygengroup:: precision_timing

.. doxygengroup:: precision_time

.. doxygengroup:: precision_clock

.. doxygengroup:: precision_clock_ptp

.. doxygengroup:: precision_pi
