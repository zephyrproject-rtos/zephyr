.. _arm_cortex_r52_coherence_tests:

Cortex-R52 kernel coherence tests
#################################

Overview
********

This test verifies :kconfig:option:`CONFIG_KERNEL_COHERENCE` on Cortex-R52
SMP systems with cached thread stacks and non-cacheable shared memory.

The ``cortex_r52_coherence`` suite checks:

* ``test_memory_placement``: kernel objects, ordinary writable data and heap
  allocations are coherent, while stacks and ``__incoherent`` data are cached.
  Initialized shared and cached data retain their values.
* ``test_same_cpu_resume``: stack and TLS contents survive 64 resumes on CPU 0.
* ``test_stack_migration``: stack and TLS contents survive 64 migrations between
  CPUs 0 and 1, repeated with the same stack to check stack reuse.

The worker checks that the MPU and both caches remain enabled. Semaphore
handshakes and explicit suspension synchronize affinity changes.

Requirements
************

The test requires at least two Cortex-R52 CPUs, an MPU, instruction and data
caches, and execution from RAM. MPU stack guards must be disabled. The SoC
must provide global exclusive monitoring for uncached memory.

The three scenarios use the default configuration,
:kconfig:option:`CONFIG_NOCACHE_MEMORY`, and
:kconfig:option:`CONFIG_STACK_SENTINEL`. The filter selects non-XIP board
configurations without overriding the board's XIP setting.

Building and running
********************

This configuration was validated with Armv8-R AEM FVP 11.32, build 19.
The board runner enables cache modeling with independent per-CPU L1 caches
so that the test exercises software coherence. The default board configuration
has four CPUs; the migration worker uses CPUs 0 and 1.

Run all three scenarios with Twister:

.. code-block:: console

   west twister -p fvp_baser_aemv8r/fvp_aemv8r_aarch32/smp \
     -T tests/arch/arm/arm_cortex_r52_coherence \
     -O twister-out/r52-coherence --inline-logs

To run only the stack-sentinel scenario, add
``-s arch.arm.cortex_r52_coherence.stack_sentinel``.

Expected results
****************

All three cases pass in each scenario, for nine passing cases in total.
Each run reports ``PROJECT EXECUTION SUCCESSFUL``.
