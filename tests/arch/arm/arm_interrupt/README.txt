Title: Test to verify code fault handling in ISR execution context
       and the behavior of irq_lock() and irq_unlock() when invoked
       from User Mode. Additional test cases verify ESF reporting and
       that null pointer dereferencing attempts are detected as CPU
       faults.

Description:

This test exercises ARM Cortex-M interrupt and fault handling. The
arm_interrupt suite verifies that the exception stack frame (ESF) is
collected and reported correctly on a fault, that system fault conditions
(spurious interrupts and exceptions, kernel oops/panic and
exception-stacking stack overflows) are handled while running in handler
mode, that user-mode threads may call irq_lock()/irq_unlock() without
faulting but cannot read or change the IRQ lock state, and that null-pointer
dereferences are caught as CPU exceptions. Individual cases are skipped when
their required features (userspace, null-pointer-exception detection) are
not enabled.

See the Doxygen comments on the individual test functions for per-case
details.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on an ARM Cortex-M QEMU platform:

    twister -p mps2/an385 -T tests/arch/arm/arm_interrupt

Or build and run a single platform directly with west:

    west build -b mps2/an385 tests/arch/arm/arm_interrupt
    west build -t run

---------------------------------------------------------------------------

Sample Output:

The test intentionally provokes CPU faults, so the console shows the
expected fault dumps captured by the test's error handler.

Running TESTSUITE arm_interrupt
===================================================================
START - test_arm_esf_collection
Testing ESF Reporting
E: ***** USAGE FAULT *****
Caught system error -- reason 0
 PASS - test_arm_esf_collection
...
===================================================================
TESTSUITE arm_interrupt succeeded
