.. _cpu_freq:

CPU Frequency Scaling Subsystem
###############################

.. toctree::
   :maxdepth: 1

   policies/index.rst
   metrics/index.rst

Overview
********

The CPU Frequency Scaling subsystem in Zephyr provides a framework for SoC's to
dynamically adjusting their processor frequency based on a monitored metric and
P-state policy algorithm. This subsystem is designed to standardize the use of
DVFS within Zephyr and allow for the kernel to transparently optimize CPU frequency,
further enhancing the performance and power efficiency of the system.

Design Goals
************

The CPU Frequency Scaling subsystem aims to provide a framework that allows for
any policy algorithm to work with any P-state driver and allow for each policy to
make use of one, or many, metrics to determine the optimal CPU frequency. The
subsystem should also be flexible enough to allow for SoC vendors to define custom
P-states, thresholds and metrics.

P-state Policies
****************
A P-state policy is an algorithm that determines what the optimal P-state is for the
CPU based on the metrics it consumes and the thresholds defined per P-state. A policy
can consume one, or many, metrics to determine the optimal CPU frequency
based on the desired statistics of the system.

See :ref:`policies <cpu_freq_policies>` for a list of standard policies.

Metrics
*******
The subsystem provides a set of metrics that can be used by a select P-state policy
for CPU freq determinations.

See :ref:`metrics <cpu_freq_metrics>` for a list of available metrics.

P-state Drivers
***************

An SoC supporting the CPU Freq subsystem must implement a P-state driver that implements
``cpu_freq_performance_state_set(struct p_state state)`` which applies the passed in
``p_state`` to the CPU when called.

An SoC must also define the available P-states in a .dts or .overlay file by implementing
the ``zephyr,p-state`` binding. The SoC may also define its own P-state binding, which
extends ``zephyr,p-state`` to include additional properties that may be used by the SoC's
P-state driver.

With the exception of an SoC custom P-state policy, the CPU Freq standard policies shall
not make use of any SoC specific properties.

Usage considerations
********************

The CPU Frequency Scaling subsystem assumes that each CPU is clocked independently and that
a P-state transitions only impacts the frequency of other CPUs in the system.

The SoC supporting CPU Freq must uphold Zephyr' requirement that the system timer remains
constant over the lifetime of the program. See :ref:`Kernel Timing <kernel_timing>` for more
information.

CPU Frequency Scaling subsystem is not compatible with SMP as of now.
