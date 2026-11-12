.. SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates
.. SPDX-License-Identifier: Apache-2.0

.. _profiling-perf-provider-cpu:

CPU performance event provider
##############################

Each ``cpuN`` provider measures the PMU on logical CPU N through the portable PMU counter and
event discovery APIs. The architecture backend supplies the supported events and implements
the PMU operations. Event names are exposed unchanged, so available names depend on the backend.
An unavailable PMU returns an error.

Configuration
*************

* :kconfig:option:`CONFIG_PROFILING_PERF_EVENT_PROVIDER_CPU`: Adds ``cpuN`` providers using the
  portable PMU API. Currently requires :kconfig:option:`CONFIG_ARM64_PMUV3` and
  :kconfig:option:`CONFIG_NUM_METAIRQ_PRIORITIES` greater than zero. SMP also requires
  :kconfig:option:`CONFIG_SCHED_CPU_MASK`.

* :kconfig:option:`CONFIG_PROFILING_PERF_EVENT_PROVIDER_CPU_STACK_SIZE`: Sets the statically
  allocated worker stack size for each configured CPU, in bytes (default 2048).

Operation
*********

Applications can enumerate supported events directly using :c:func:`pmu_num_events` and
:c:func:`pmu_event_get`. Each index identifies a position in the supported event list, and each
record supplies a name with static lifetime and a hardware event selector. Discovery can run before
PMU initialization and during measurement without changing PMU state. The caller must have access
to the PMU registers and remain on the same CPU throughout enumeration; CPUs can support different
events.

Provider listing and lookup use only read-only event discovery, without initializing the PMU or
modifying counters. On each CPU, initialization is deferred until the first event is prepared;
it may reset counters and perform backend calibration.

PMU operations run synchronously through a :ref:`meta-IRQ worker <metairq_priorities>` pinned to
the owning CPU. Workers use the highest reserved meta-IRQ priority so requests can preempt
ordinary cooperative and scheduler-locked threads. Ordinary workloads must use priorities below
the reserved meta-IRQ range. Interrupt masking or a non-yielding thread at the same meta-IRQ
priority can still delay requests.

Requests to a CPU that has not started return ``-ENODEV`` without waiting for its worker.
The provider becomes usable after the CPU starts. This startup check does not provide CPU hotplug
support.

The caller can start and stop on different CPUs. Each prepare callback configures and resets one
disabled counter.
After all events are prepared, the first start callback for a CPU enables all its prepared
counters, then starts the PMU. All events use zero baselines. The first stop callback stops that
CPU's PMU and saves all counts; later stop callbacks return the saved count for each event.
Boundaries across different CPUs are not simultaneous. Counts cover all execution on the measured
CPU, including interrupts and provider worker overhead; they are not per-thread counts.

Each selected event consumes one programmable counter. Per-CPU capacity is limited by the hardware,
:kconfig:option:`CONFIG_PMU_MAX_COUNTERS`, and
:kconfig:option:`CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS`. Larger selections fail with ``-ENOSPC``.
A sticky counter overflow flag causes ``-EOVERFLOW`` in the event status. Prepared assignments are
released through ``stop()`` on failed startup, including when counting never began.

The provider requires exclusive PMU access: applications must not use the raw PMU API or reset
counters during a session. CPUs must remain online, and power states that lose PMU state must
not be entered during a session. CPU hotplug and suspend/resume restoration are not supported.
The provider and its worker storage are omitted when its Kconfig option is disabled.

Arm64 PMUv3
***********

The Arm64 PMUv3 backend exposes names such as ``CPU_CYCLES``, ``SW_INCR``, and ``INST_RETIRED``.
It filters its predefined list of common architectural events using ``PMCEID0_EL0[31:0]``.
Events reported by ``PMCEID1_EL0`` or ``PMCEID0_EL0[63:32]``, and implementation-defined events,
are not currently enumerated. The provider uses a programmable counter for each selected event,
including ``CPU_CYCLES``.
