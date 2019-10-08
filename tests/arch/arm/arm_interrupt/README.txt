Title: Test to verify code fault handling in ISR execution context (ARM Only)

Description:

This test verifies that we can handle system fault conditions
while running in handler mode (i.e. in an ISR). Only for ARM
Cortex-M targets.

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
***** Booting Zephyr OS build zephyr-v2.0.0-1066-ga087055d4e3d *****
 Running test suite arm_interrupt
 ===================================================================
 starting test - test_arm_interrupt
 Available IRQ line: 25
 E: ***** HARD FAULT *****
 E: Z_ARCH_EXCEPT with reason 3

 E: r0/a1:  0x00000003  r1/a2:  0x20001240  r2/a3:  0x00000003
 E: r3/a4:  0x20001098 r12/ip:  0x00000000 r14/lr:  0x000012c9
 E:  xpsr:  0x01000029
 E: Faulting instruction address (r15/pc): 0x000003de
 E: >>> ZEPHYR FATAL ERROR 3: Kernel oops
 E: Current thread: 0x20000058 (unknown)
 Caught system error -- reason 3
 E: Fault during interrupt handling

 E: ***** HARD FAULT *****
 E: Z_ARCH_EXCEPT with reason 4

 E: r0/a1:  0x00000004  r1/a2:  0x20001240  r2/a3:  0x00000004
 E: r3/a4:  0x20001098 r12/ip:  0x00000000 r14/lr:  0x000012c9
 E:  xpsr:  0x01000029
 E: Faulting instruction address (r15/pc): 0x000003e8
 E: >>> ZEPHYR FATAL ERROR 4: Kernel panic
 E: Current thread: 0x20000058 (unknown)
 Caught system error -- reason 4
 E: Fault during interrupt handling

 PASS - test_arm_interrupt
 ===================================================================
 Test suite arm_interrupt succeeded
 ===================================================================
 PROJECT EXECUTION SUCCESSFUL
