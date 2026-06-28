.. _side_channel_defense_policy:

Side-Channel Defense CPU Frequency Scaling Policy
#################################################

Overview
########

The side-channel defense policy is a CPU frequency scaling policy intended to reduce
the effectiveness of side-channel attacks that rely on non-constant-time functions
by introducing randomized CPU frequency changes during application execution. This
essentially injects clock noise into the system, making it harder to profile for
side-channel attackers.

The objective is to decrease the repeatability of execution timing measurements,
making it more difficult for an attacker to distinguish small timing differences
that may reveal sensitive information.

Randomizing the SoC's operating frequency affects not only the wall-clock execution
time of certain functions, but also the device's power consumption. This adds an
additional layer of security that can reduce the effectiveness of adversaries
attempting to profile or probe the system.

Use Cases
#########

This policy is intended for applications where resistance to timing-based
side-channel attacks is more important than deterministic execution time or
optimal power efficiency.

Of course, systems performing cryptographic operations should rely on more robust
methods of side-channel resistance. This policy provides an additional layer of
defense that may help mitigate unintended code paths that are not implemented in
constant time.

Configuration
#############

Enable the CPU frequency subsystem and select the side-channel defense policy:

.. code-block:: kconfig

CONFIG_CPU_FREQ=y
CONFIG_CPU_FREQ_POLICY_SIDE_CHANNEL_DEFENSE=y

The policy uses the standard CPU frequency subsystem update interval,
configured with:

.. code-block:: kconfig

CONFIG_CPU_FREQ_INTERVAL_MS=<interval>

Smaller update intervals increase timing variability.

Random Number Generation
########################

The policy selects performance states using :c:func:`sys_rand32_get()`.
Selecting ``CONFIG_CPU_FREQ_POLICY_SIDE_CHANNEL_DEFENSE`` enables the
entropy driver on platforms with a hardware entropy source. Also enable a
PRNG backend in the application configuration, for example:

.. code-block:: kconfig

CONFIG_XOSHIRO_RANDOM_GENERATOR=y

Security Considerations
#######################

This policy introduces noise into execution timing but, by itself, does not
guarantee protection against timing attacks.

It should be viewed as a defense-in-depth mechanism rather than a replacement
for secure software design.

Limitations
###########

This policy has several limitations:

* It does not eliminate timing side channels
* It cannot compensate for fundamentally insecure software
* It may reduce overall system performance
* It may increase power consumption due to frequent performance state changes
