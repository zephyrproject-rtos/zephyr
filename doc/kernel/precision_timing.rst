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
:c:struct:`net_ptp_time` and :c:type:`precision_time_t`; it does not add generic
capability discovery or synchronization state.

PI controller
*************

:c:struct:`precision_pi` is an instance-based proportional-integral controller.
Each instance stores its own gains, limits and integral term.
:c:func:`precision_pi_update_interval` takes an error sample and the interval
since the previous sample; the update is equivalent to:

.. code-block:: c

   integral = clamp(integral + (ki / interval) * error, integral_limit);
   output = clamp((kp / interval) * error + integral, output_limit);

The gains are given for an interval of 1. When the plant integrates the output
over the interval, as a clock does with a rate correction, the loop gain per
sample is then the same for any interval. :c:func:`precision_pi_update` is the
same update with an interval of 1.

:c:func:`precision_pi_set_limits` sets the two limits; a limit of 0 disables it,
which is the state after :c:func:`precision_pi_init`. While the output is at its
limit, the integral term keeps its value instead of moving further towards the
limit, so it does not wind up.

The controller does not decide whether an error should be stepped, rejected, or
used for rate adjustment. It also does not track synchronization, acquisition,
lock, holdover, source timeout, or clock faults. Callers own those decisions and
reset the accumulated integral term when their policy requires it.

The PTP and gPTP integrations initialize their controllers from
:kconfig:option:`CONFIG_PRECISION_TIMING_PI_KP` and
:kconfig:option:`CONFIG_PRECISION_TIMING_PI_KI`, with the limits from
:kconfig:option:`CONFIG_PRECISION_TIMING_PI_INTEGRAL_LIMIT_PPM` and
:kconfig:option:`CONFIG_PRECISION_TIMING_PI_OUTPUT_LIMIT_PPM`. The integer gain
values express the gains in thousandths. Direct users of
:c:func:`precision_pi_init` may provide different gains for each controller
instance.

The PTP servo scales its gains by the time since its previous update when
:kconfig:option:`CONFIG_PTP_SERVO_SCALE_GAINS_BY_SYNC_INTERVAL` is enabled.
:kconfig:option:`CONFIG_PTP_SERVO_STEP_THRESHOLD_MS` and
:kconfig:option:`CONFIG_PTP_SERVO_OUTLIER_THRESHOLD_MS` set the offsets above
which the PTP servo steps the clock and, while locked, rejects a sample. Before
the servo locks, it also steps offsets above a fixed 100 ms.

Scheduled clock output
**********************

:kconfig:option:`CONFIG_PRECISION_CLOCK_OUTPUT` adds optional scheduled-output
operations to :c:struct:`precision_clock`. Output capabilities are reported per
channel with :c:func:`precision_clock_output_get_caps`; there is no generic
capability query for the clock itself. Each missing output callback returns
``-ENOTSUP`` independently.

Scheduled output and the autonomous PPS service are experimental features:
:kconfig:option:`CONFIG_PRECISION_CLOCK_OUTPUT` and
:kconfig:option:`CONFIG_PRECISION_PPS_OUTPUT` select
:kconfig:option:`CONFIG_EXPERIMENTAL`. The shared
:kconfig:option:`CONFIG_PRECISION_TIMING` core does not select it, so enabling
gPTP without either output feature does not introduce an experimental-feature
dependency.

A channel supports periodic waveforms.
:c:func:`precision_clock_output_start_waveform`
accepts an absolute first rising edge, a period, and either a provider-default
or exact high-pulse width. All absolute times are :c:type:`precision_time_t`
values interpreted in the owning clock's timescale. Periods and widths are
nanoseconds and must meet the limits and resolution reported for the channel.

Channel indices are provider-defined and zero-based; they do not imply
output-only hardware resources. A channel may share hardware with another
device function, such as input capture, or be reserved by the driver.
Starting a waveform may return ``-EBUSY`` if the channel is already configured
for output, is in use by another device function, or is reserved by the driver.
The driver arbitrates access to shared channels.

:c:func:`precision_clock_output_next_start_time` finds the next period-aligned
start that satisfies a requested lead time using checked arithmetic. Use
:c:func:`precision_clock_output_get_status` to inspect an accepted
configuration, and :c:func:`precision_clock_output_stop` to disable a channel.
``configured`` describes provider ownership; ``hardware_active`` is meaningful
only when ``hardware_active_valid`` is set. It reports whether the hardware
output generator is armed or running for the accepted configuration, including
before a future first edge and during the low portion of a waveform. It does
not report the instantaneous pin level or whether an edge is occurring. When
hardware state is unobservable, the accepted configuration remains the source
of truth.

The generated waveform follows its underlying clock. A hard clock set or phase
step can cross an edge or make the waveform discontinuous, so the clock owner
must stop and rearm output when its policy requires that. Scheduled output does
not imply PTP lock, UTC correctness, grandmaster health, or PPS accuracy. PPS
input and external timestamp capture are outside this API.

Driver providers
================

The driver-facing :c:struct:`precision_clock_output_provider` contract is in
:zephyr_file:`include/zephyr/drivers/precision_clock_output.h`. It uses raw
nanosecond values in the device clock's own timescale. A PTP clock driver can
reference a provider, and :c:struct:`precision_clock_ptp_adapter` dispatches its
operations without probing it during adapter initialization.

Provider operations are called only from thread context and may block.
Providers may document additional, more restrictive context requirements.

The extension deliberately uses a protocol-neutral contract rather than adding
a ``ptp_clock`` periodic-output call: non-PTP clocks can reuse the same provider
interface and output data types. The PTP driver API only forward declares the
provider, so drivers without output support do not need precision-timing headers
or runtime support. Output providers use the public
:c:struct:`precision_clock_output_waveform_config` and
:c:struct:`precision_clock_output_status` directly; the PTP adapter needs neither
duplicate structures nor member-by-member conversions.

Autonomous PPS output
*********************

:kconfig:option:`CONFIG_PRECISION_PPS_OUTPUT` enables an instance-based service
that maintains a fixed one-second waveform on a caller-selected precision clock
and channel. It requires multithreading and a system clock. It is an optional
layer over direct scheduled-output operations and does not discover clocks or
inspect synchronization protocol state.

Each caller-owned :c:struct:`precision_pps_output` is initialized with a
:c:struct:`precision_pps_output_config`. The configuration selects the channel,
width policy, start guard, hard-step threshold, and polling interval. Start
queries the channel limits, then polls the clock and output on a dedicated
subsystem workqueue. The workqueue is started lazily on the first successful
:c:func:`precision_pps_output_start` and is not created by an init function, so
enabling the option without ever starting an instance does not spawn the
workqueue thread. The workqueue stack and priority are configured with
:kconfig:option:`CONFIG_PRECISION_PPS_OUTPUT_WORKQUEUE_STACK_SIZE` and
:kconfig:option:`CONFIG_PRECISION_PPS_OUTPUT_WORKQUEUE_PRIORITY`.

Poll-time provider operations and callbacks across instances are serialized on
that shared workqueue. The capability query during start and the provider stop
during stop run on the caller's thread and can overlap another instance's poll;
the workqueue does not serialize lifecycle operations across instances.

The service can adopt a matching configured waveform. A detected clock step,
missing or mismatched configuration, or reported inactive output causes a stop
and rearm at a whole-second boundary. Transient read and status errors are
reported and retried. The callback receives event flags and a coherent
:c:struct:`precision_pps_output_state` snapshot; callers can obtain the same
snapshot with :c:func:`precision_pps_output_state_get`.

Clock reads are bracketed by monotonic timestamps, including the fresh read
after stopping an output. The hard-step threshold applies only to the elapsed-time
discrepancy that remains after accounting for both reads' sampling intervals.
This prevents slow or preempted reads from falsely triggering a stop and rearm.
``continuity_error_ns`` reports that signed minimum discrepancy, not an exact
step measurement. A real step hidden within the sampling uncertainty may go
undetected.

A started instance requires exclusive management of its clock/channel pair.
:c:func:`precision_pps_output_stop` synchronously cancels polling and stops the
provider. Cancellation waits for a poll that is already running, including its
callback, to finish on the workqueue before the provider channel is stopped, so
the callback's final access to its user data is complete when stop returns. If
provider stop fails, the instance remains started so its clock lifetime cannot
be released accidentally. Calling stop from the instance's own callback returns
``-EDEADLK``. The instance embeds the work-sync object used for this
cancellation, so when :kconfig:option:`CONFIG_KERNEL_COHERENCE` is enabled the
instance storage must reside in coherent memory.

Shell control
*************

:kconfig:option:`CONFIG_PRECISION_TIMING_SHELL` enables the ``precision_clock``
command. Clocks are registered statically at build time with
:c:macro:`PRECISION_TIMING_SHELL_REGISTER`, which creates an immutable,
linker-collected entry referencing a lifetime-static :c:struct:`precision_clock`.
There is no runtime register or unregister API.

For example, a PTP adapter can expose its embedded clock at file scope:

.. code-block:: c

   static struct precision_clock_ptp_adapter phc_adapter;

   PRECISION_TIMING_SHELL_REGISTER(phc, &phc_adapter.precision_clk);

Initialize the adapter before allowing concurrent shell access, and keep it
valid for the lifetime of the application. Entries cannot be removed at runtime.

The shell can list and read registered clocks:

.. code-block:: console

   precision_clock list
   precision_clock get <name>

When scheduled output is enabled, these commands operate on a channel:

.. code-block:: console

   precision_clock output caps <name> <channel>
   precision_clock output get <name> <channel>
   precision_clock output waveform <name> <channel> <first_rising_ns> <period_ns> [pulse_width_ns]
   precision_clock output stop <name> <channel>
   precision_clock pps start <name> <channel> [pulse_width_ns]
   precision_clock pps stop <name> <channel>

The absolute time arguments are nanoseconds in the registered clock's timescale.
The PPS convenience command chooses a whole-second start with an additional
scheduling guard and arms the waveform once. It does not run the autonomous
:c:struct:`precision_pps_output` service: there is no hard-step monitoring,
health polling, automatic rearming, or output-loss recovery. It does not check
protocol synchronization state. Use :c:struct:`precision_pps_output` for
continuous PPS with automatic recovery.

The PPS command checks that the channel is unconfigured before attempting a
start. An existing configuration returns ``-EBUSY``; a failed status query
returns its error without starting or stopping the output. Stop the channel
explicitly before reconfiguring it. Stopping a channel that a running
:c:struct:`precision_pps_output` instance owns does not stop the service: the
service rearms the channel on its next poll.

If the first start then fails with ``-ETIME``, the command stops the output to
release any retained configuration before making one retry with a fresh clock
reading. A failed stop is returned to the caller and prevents that retry.

Protocol integration
********************

PTP and the gPTP default clock-update path keep their existing policy and use a
:c:struct:`precision_pi` for the shared calculation. They initialize a
PTP-clock adapter once and use :c:struct:`precision_clock` operations to access
the PHC. Scheduled output remains independent of those protocol state machines.

Sample
******

The :zephyr:code-sample:`precision_timing` sample demonstrates checked time
arithmetic, a software-backed precision clock, and PI-driven rate adjustment.

API reference
*************

.. doxygengroup:: precision_timing

.. doxygengroup:: precision_time

.. doxygengroup:: precision_clock

.. doxygengroup:: precision_clock_output_provider

.. doxygengroup:: precision_pi

.. doxygengroup:: precision_clock_ptp

.. doxygengroup:: precision_pps_output

.. doxygengroup:: precision_timing_shell
