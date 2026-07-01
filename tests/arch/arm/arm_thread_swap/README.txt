Title: Test suite to verify the thread-swap (context-switch) and system-calls
mechanisms (ARM Only)

Description:

This test verifies the ARM Cortex-M thread context-switch and user
system-call mechanisms. It is supported on ARM Cortex-M Baseline and
Mainline targets (the system-call cases additionally require user-space
support).

The arm_thread_swap suite verifies that the callee-saved (and FP
callee-saved) registers, the execution priority (BASEPRI), the swap return
value and the thread mode variable are correctly saved and restored across
both indirect (pending PendSV) and direct (arch_swap()) context switches.
It further verifies the user system-call path: the mode variable tracks the
current execution mode, system calls run on the privileged stack with
correct stack-pointer-limit checking, and the kernel scrubs the caller-saved
registers (r0-r3) on the syscall return path so no kernel value leaks back
to the calling user thread. The system-call cases are skipped when user
space is not enabled.

See the Doxygen comments on the individual test functions for per-case
details.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on a QEMU ARM Cortex-M target:

    twister -p mps2/an385 -T tests/arch/arm/arm_thread_swap

Or build and run a single platform directly with west:

    west build -b mps2/an385 tests/arch/arm/arm_thread_swap
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE arm_thread_swap
===================================================================
START - test_arm_thread_swap
 PASS - test_arm_thread_swap
...
===================================================================
TESTSUITE arm_thread_swap succeeded
