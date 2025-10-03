.. _cpu_freq:

CPU Frequency Scaling
#####################

.. toctree::
   :maxdepth: 1

   policies/index.rst

Overview
********

The CPU Frequency Scaling subsystem in Zephyr provides a framework for SoC's to dynamically adjust
their processor frequency based on a monitored metric and performance state (p-state) policy
algorithm.

Design Goals
************

The CPU Frequency Scaling subsystem aims to provide a framework that allows for any policy algorithm
to work with any p-state driver and allows for each policy to make use of one, or many, metrics to
determine an optimal CPU frequency. The subsystem should be flexible enough to allow for SoC vendors
to define custom p-states, thresholds and metrics.

P-state Policies
****************

A p-state policy is an algorithm that determines what the optimal p-state is for the CPU based on
the metrics it consumes and the thresholds defined per p-state. A policy can consume one, or many,
metrics to determine the optimal CPU frequency based on the desired statistics of the system.

See :ref:`policies <cpu_freq_policies>` for a list of standard policies.

Metrics
*******

A P-state policy should include one or more metrics to base decisions. Examples of metrics could
include percent CPU load, SoC temperature, etc.

For an example of a metric in use, see the :ref:`on_demand <on_demand_policy>` policy.

P-state Drivers
***************

A SoC supporting the CPU Freq subsystem must implement a p-state driver that implements
:c:func:`cpu_freq_pstate_set` which applies the passed in ``p_state`` to
the CPU when called.

A SoC must also provide the available p-states in devicetree by having a
:dtcompatible:`zephyr,pstate` compatible node. The SoC may also define its own p-state binding,
which extends :dtcompatible:`zephyr,pstate` to include additional properties that may be used by
the SoC's p-state driver.

Usage considerations
********************

The CPU Frequency Scaling subsystem assumes that each CPU is clocked independently and that a
p-state transition does not impact an unrelated CPU of the SoC.

The SoC supporting CPU Freq must uphold Zephyr's requirement that the system timer remains constant
over the lifetime of the program. See :ref:`Kernel Timing <kernel_timing>` for more information.

The CPU Frequency Scaling subsystem runs as a handler function to a ``k_timer``, which means it runs
in interrupt context (IRQ). The SoC p-state driver must ensure that its implementation of
:c:func:`cpu_freq_pstate_set` is IRQ context safe. If a p-state transition
cannot be completed reasonably in an IRQ context, it is recommended that the p-state driver of the
SoC implements its task as a workqueue item.

CPU Frequency Scaling subsystem is not compatible with SMP as of now since the thread can migrate
between cores during execution, causing per-CPU metrics to be attributed to the wrong CPU.
