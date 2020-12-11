Title: Test suite to verify the thread-swap (context-switch) and system-calls
mechanisms (ARM Only)

Description:

Thread-swap test:

This test verifies that the ARM thread context-switch mechanism
behaves as expected. In particular, the test verifies that:
- the callee-saved registers are saved and restored, properly,
  at thread swap-out and swap-in, respectively
- the floating-point callee-saved registers are saved and
  restored, properly, at thread swap-out and swap-in, respectively,
  when the thread is using the floating-point registers
- the thread execution priority (BASEPRI) is saved and restored,
  properly, at thread context-switch
- the swap return value can be set and will be return, properly,
  at thread swap-in
- the mode variable (when building with support for either user
  space or FP shared registers) is saved and restored properly.
- the CPU registers are scrubbed after system call

Notes:
  The test verifies the correct behavior of the thread context-switch,
  when it is triggered indirectly (by setting the PendSV interrupt
  to pending state), as well as when the thread itself triggers its
  swap-out (by calling arch_swap(.)).

  The test is currently supported in ARM Cortex-M Baseline and Mainline
  targets.

Syscalls test:

This test verifies that the ARM mechanism for user system calls
behaves as expected. In particular, the test verifies that:
- the mode variable with respect to the user mode flag always indicates
  the mode in which a user thread is currently executing.
- threads in system calls are using the privileged thread stack
- stack pointer limit checking mechanism behaves as expected for
  user threads in PRIV mode and supervisor threads

The test is currently supported in ARM Cortex-M Baseline and Mainline
targets with support for user space.

---------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed on QEMU as
follows:

    ninja/make run

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    ninja/make clean    # discard results of previous builds
                        # but keep existing configuration info
or
    ninja/make pristine # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

***** Booting Zephyr OS build zephyr-v1.14.0-1726-gb95a71960622 *****
Running test suite arm_thread_swap
===================================================================
starting test - test_arm_thread_swap
PASS - test_arm_thread_swap
===================================================================
Test suite arm_thread_swap succeeded
Running test suite arm_syscalls
===================================================================
starting test - test_arm_syscalls
Available IRQ line: 68
USR Thread: IRQ Line: 68
PASS - test_arm_syscalls
===================================================================
Test suite arm_syscalls succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
