.. _cpu_freq:

CPU Frequency Scaling
#####################

.. toctree::
   :maxdepth: 1

   policies/index.rst

Overview
********

The CPU Frequency Scaling subsystem in Zephyr provides a framework for SoC's to dynamically adjust
their processor frequency based on a monitored metric and performance state (P-state) policy
algorithm.

Design Goals
************

The CPU Frequency Scaling subsystem aims to provide a framework that allows for any policy algorithm
to work with any P-state driver and allows for each policy to make use of one, or many, metrics to
determine an optimal CPU frequency. The subsystem should be flexible enough to allow for SoC vendors
to define custom P-states, thresholds and metrics.

P-state Policies
****************

A P-state policy is an algorithm that determines what the optimal P-state is for the CPU based on
the metrics it consumes and the thresholds defined per P-state. A policy can consume one, or many,
metrics to determine the optimal CPU frequency based on the desired statistics of the system.

See :ref:`policies <cpu_freq_policies>` for a list of standard policies.

Metrics
*******

A P-state policy should include one or more metrics to base decisions. Examples of metrics could
include percent CPU load, SoC temperature, etc.

For an example of a metric in use, see the :ref:`on_demand <on_demand_policy>` policy.

P-state Drivers
***************

A SoC supporting the CPU Frequency Scaling subsystem must implement a P-state driver that implements
:c:func:`cpu_freq_pstate_set` which applies the passed in ``p_state`` to
the CPU when called.

A SoC must also provide the available P-states in devicetree by having a
:dtcompatible:`zephyr,pstate` compatible node. The SoC may also define its own P-state binding,
which extends :dtcompatible:`zephyr,pstate` to include additional properties that may be used by
the SoC's P-state driver.

Usage considerations
********************

The CPU Frequency Scaling subsystem is designed to work on both UP and SMP system. On SMP systems,
it is assumed by default that each of the CPUs are clocked at the same rate. Thus, should one CPU
undergo a P-state transition, then all other CPUs will also undergo the same P-state transition.
This can be overridden by the SoC by enabling the :kconfig:option:`CONFIG_CPU_FREQ_PER_CPU_SCALING`
configuration option to allow each CPU to be clocked independently.

The SoC supporting CPU Frequency Scaling must uphold Zephyr's requirement that the system timer
frequency remains steady over the lifetime of the program. See :ref:`Kernel Timing <kernel_timing>`
for more information.

The CPU Frequency Scaling subsystem runs as a handler function to a ``k_timer``, which means it runs
in interrupt context (IRQ). The SoC P-state driver must ensure that its implementation of
:c:func:`cpu_freq_pstate_set` is IRQ context safe. If a P-state transition
cannot be completed reasonably in an IRQ context, it is recommended that the P-state driver of the
SoC implements its task as a workqueue item.
