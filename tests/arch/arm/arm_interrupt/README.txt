Title: Test to verify code fault handling in ISR execution context
       and the behavior of irq_lock() and irq_unlock() when invoked
       from User Mode. (ARM Only)

Description:

The first test verifies that we can handle system fault conditions
while running in handler mode (i.e. in an ISR). Only for ARM
Cortex-M targets.

The test also verifies
- the behavior of the spurious interrupt handler, as well as the
  ARM spurious exception handler.
- the ability of the Cortex-M fault handling mechanism to detect
  stack overflow errors explicitly due to exception frame context
  stacking (that is when the processor reports only the Stacking
  Error and not an additional Data Access Violation error with a
  valid corresponding MMFAR address value).

The second test verifies that threads in user mode, despite being able to call
the irq_lock() and irq_unlock() functions without triggering a CPU fault,
they won't be able to read or modify the current IRQ locking status.

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
*** Booting Zephyr OS build v2.3.0-rc1-295-g630f69889088  ***
Running test suite arm_interrupt
===================================================================
starting test - test_arm_interrupt
Available IRQ line: 70
E: >>> ZEPHYR FATAL ERROR 1: Unhandled interrupt on CPU 0
E: Current thread: 0x20400190 (unknown)
Caught system error -- reason 1
E: r0/a1:  0x00000003  r1/a2:  0x20401d6c  r2/a3:  0x00000003
E: r3/a4:  0x20401af8 r12/ip:  0x0295a8d1 r14/lr:  0x00401ad3
E:  xpsr:  0x61000056
E: Faulting instruction address (r15/pc): 0x00400622
E: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
E: Fault during interrupt handling

E: Current thread: 0x20400190 (unknown)
Caught system error -- reason 3
E: r0/a1:  0x00000004  r1/a2:  0x20401d6c  r2/a3:  0x00000004
E: r3/a4:  0x20401af8 r12/ip:  0x00000000 r14/lr:  0x00401ad3
E:  xpsr:  0x61000056
E: Faulting instruction address (r15/pc): 0x00400640
E: >>> ZEPHYR FATAL ERROR 4: Kernel panic on CPU 0
E: Fault during interrupt handling

E: Current thread: 0x20400190 (unknown)
Caught system error -- reason 4
ASSERTION FAIL [0] @ ../src/arm_interrupt.c:49
        Intentional assert

E: r0/a1:  0x00000004  r1/a2:  0x00000031  r2/a3:  0x80000000
E: r3/a4:  0x00000056 r12/ip:  0x00000000 r14/lr:  0x00401ad3
E:  xpsr:  0x41000056
E: Faulting instruction address (r15/pc): 0x00409b34
E: >>> ZEPHYR FATAL ERROR 4: Kernel panic on CPU 0
E: Fault during interrupt handling

E: Current thread: 0x20400190 (unknown)
Caught system error -- reason 4
E: ***** MPU FAULT *****
E:   Stacking error (context area might be not valid)
E: r0/a1:  0x2e706106  r1/a2:  0x739e3f03  r2/a3:  0xf1c99979
E: r3/a4:  0xe9973b26 r12/ip:  0x87d3b16f r14/lr:  0x01c98b2b
E:  xpsr:  0x80608800
E: Faulting instruction address (r15/pc): 0x885e4061
E: >>> ZEPHYR FATAL ERROR 2: Stack overflow on CPU 0
E: Current thread: 0x20400190 (unknown)
Caught system error -- reason 2
PASS - test_arm_interrupt
===================================================================
starting test - test_arm_user_interrupt
PASS - test_arm_user_interrupt
===================================================================
Test suite arm_interrupt succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
