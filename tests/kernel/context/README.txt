Title: Context and IRQ APIs

Description:

This test verifies that the kernel CPU, context and interrupt APIs operate
as expected. It is split into three test suites:

1. context
   - test_interrupts exercises irq_lock()/irq_unlock() and irq_offload():
     interrupts stay masked while locked and the offloaded ISR runs in
     interrupt context.
   - test_arch_cpu_irqs_are_enabled checks the arch helper reporting the
     current interrupt-enable state.
   - test_irq_lock_nested verifies that nested irq_lock()/irq_unlock() only
     re-enable interrupts once fully unwound.
   - test_k_can_yield reports whether the current context is allowed to
     yield.
   - test_ctx_thread checks k_current_get() and k_is_in_isr() from both
     thread and ISR context.

2. context_one_cpu (single-CPU only)
   - test_timer_interrupts confirms the tick timer keeps advancing ticks.
   - test_busy_wait validates k_busy_wait() spins for the requested time.
   - test_k_sleep checks that k_sleep() blocks for the requested duration.
   - test_k_yield and test_ctx_coop_thread exercise k_yield() and
     cooperative thread scheduling with k_thread_create().

3. context_cpu_idle
   - test_cpu_idle and test_cpu_idle_atomic verify that k_cpu_idle() and
     k_cpu_atomic_idle() halt the CPU until the next interrupt wakes it.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/kernel/context

Or build and run a single platform directly with west:

    west build -b qemu_x86 tests/kernel/context
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE context
===================================================================
START - test_arch_cpu_irqs_are_enabled
 PASS - test_arch_cpu_irqs_are_enabled
===================================================================
START - test_ctx_thread
 PASS - test_ctx_thread
===================================================================
START - test_interrupts
 PASS - test_interrupts
===================================================================
START - test_irq_lock_nested
 PASS - test_irq_lock_nested
===================================================================
START - test_k_can_yield
 PASS - test_k_can_yield
===================================================================
TESTSUITE context succeeded

Running TESTSUITE context_cpu_idle
===================================================================
START - test_cpu_idle
 PASS - test_cpu_idle
===================================================================
START - test_cpu_idle_atomic
 PASS - test_cpu_idle_atomic
===================================================================
TESTSUITE context_cpu_idle succeeded

Running TESTSUITE context_one_cpu
===================================================================
START - test_busy_wait
 PASS - test_busy_wait
===================================================================
START - test_ctx_coop_thread
 PASS - test_ctx_coop_thread
===================================================================
START - test_k_sleep
 PASS - test_k_sleep
===================================================================
START - test_k_yield
 PASS - test_k_yield
===================================================================
START - test_timer_interrupts
 PASS - test_timer_interrupts
===================================================================
TESTSUITE context_one_cpu succeeded
