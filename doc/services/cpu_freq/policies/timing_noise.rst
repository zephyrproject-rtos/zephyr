.. _timing_noise_policy:

Timing Noise CPU Frequency Scaling Policy
#########################################

Overview
########

The timing noise policy is a CPU frequency scaling policy that periodically
selects a random performance state (P-state). The intent is to inject timing
variability by jittering the CPU clock, which can disrupt naive cycle-count
or wall-clock measurements that assume a stable frequency.

This is a randomized P-state policy. It is not a general side-channel
countermeasure.

Primary Mitigation
##################

Constant-time and constant-flow implementations that have been reviewed for
side-channel leakage remain the primary mitigation against timing attacks.
This policy does not replace those practices. At best, it may complicate a
particular class of measurement.

Attacker Model
##############

This policy considers an adversary that:

* Observes relative execution time via cycle counters, wall-clock timers, or
  similar software-visible timestamps
* Relies on a stable CPU frequency so that small differences in instruction
  count or data-dependent paths remain distinguishable across trials

Randomized P-state selection does not defeat such an attack. Given enough
samples, an attacker can often average through the noise. The policy only
makes those measurements less convenient and less repeatable.

Use Cases
#########

This policy may be useful when an application wants additional timing
variability as a secondary layer on top of already sound software. It should
be treated as a way to disrupt certain analyses, not as a security solution
on its own.

It is a poor fit for:

* Hard real-time systems that need deterministic execution time
* Safety-related systems where timing predictability is part of the safety case
* Latency-sensitive workloads that cannot tolerate unexpected slow P-states
* Strict power-budget designs, since frequent P-state changes may increase
  average power consumption

Configuration
#############

Enable the CPU frequency subsystem and select the timing noise policy:

.. code-block:: kconfig

   CONFIG_CPU_FREQ=y
   CONFIG_CPU_FREQ_POLICY_TIMING_NOISE=y

The policy uses the standard CPU frequency subsystem update interval,
configured with:

.. code-block:: kconfig

   CONFIG_CPU_FREQ_INTERVAL_MS=<interval>

Smaller update intervals increase timing variability, but also increase the
number of frequency transitions.

Random Number Generation
########################

The policy selects performance states using :c:func:`sys_rand32_get()`.
Selecting ``CONFIG_CPU_FREQ_POLICY_TIMING_NOISE`` enables the entropy driver
on platforms with a hardware entropy source. Prefer routing random draws
through that driver when a TRNG is available:

.. code-block:: kconfig

   CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR=y

Limitations
###########

* It does not eliminate timing side channels
* It cannot compensate for fundamentally insecure software
* It may reduce overall system performance
* It may increase power consumption due to frequent performance state changes
* It offers no guarantees against a determined or well-instrumented attacker

For an example of the timing noise policy, refer to the
:zephyr:code-sample:`cpu_freq_timing_noise` sample.
