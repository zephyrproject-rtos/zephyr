Title: Test to verify the thread-swap (context-switch) mechanism (ARM Only)

Description:

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

Notes:
  The test verifies the correct behavior of the thread context-switch,
  when it is triggered indirectly (by setting the PendSV interrupt
  to pending state), as well as when the thread itself triggers its
  swap-out (by calling z_arch_swap(.)).

  The test is currently supported in ARM Cortex-M Baseline and Mainline
  targets.


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
===================================================================
PROJECT EXECUTION SUCCESSFUL
