Title: Test to verify code fault handling in ISR execution context
       and the behavior of irq_lock() and irq_unlock() when invoked
       from User Mode. An additional test case verifies that null
       pointer dereferencing attempts are detected and interpreted
       as CPU faults. Tests supported only on Cortex-M architecture.

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

An additional test case verifies that null pointer dereferencing (read)
attempts are caught and successfully trigger a CPU exception in the case
the read is attempted by privileged code.

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
*** Booting Zephyr OS build zephyr-v2.4.0-3490-ga44a42e3f4d5  ***
Running test suite arm_interrupt
===================================================================
START - test_arm_null_pointer_exception
E: ***** Debug monitor exception *****
Null-pointer exception?
E: r0/a1:  0x20000000  r1/a2:  0x00000000  r2/a3:  0x20001e40
E: r3/a4:  0x00003109 r12/ip:  0xfabf33ff r14/lr:  0x00003e7f
E:  xpsr:  0x41000000
E: Faulting instruction address (r15/pc): 0x00000f34
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x20000148 (unknown)
Caught system error -- reason 0
 PASS - test_arm_null_pointer_exception
===================================================================
START - test_arm_interrupt
Available IRQ line: 68
E: >>> ZEPHYR FATAL ERROR 1: Unhandled interrupt on CPU 0
E: Current thread: 0x20000148 (unknown)
Caught system error -- reason 1
E: r0/a1:  0x00000003  r1/a2:  0x200020b8  r2/a3:  0x00000003
E: r3/a4:  0x20001e40 r12/ip:  0x00000000 r14/lr:  0x00002f47
E:  xpsr:  0x61000054
E: Faulting instruction address (r15/pc): 0x000009a6
E: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
E: Fault during interrupt handling

E: Current thread: 0x20000148 (unknown)
Caught system error -- reason 3
E: r0/a1:  0x00000004  r1/a2:  0x200020b8  r2/a3:  0x00000004
E: r3/a4:  0x20001e40 r12/ip:  0x00000000 r14/lr:  0x00002f47
E:  xpsr:  0x61000054
E: Faulting instruction address (r15/pc): 0x000009c4
E: >>> ZEPHYR FATAL ERROR 4: Kernel panic on CPU 0
E: Fault during interrupt handling

E: Current thread: 0x20000148 (unknown)
Caught system error -- reason 4
ASSERTION FAIL [0] @ ../src/arm_interrupt.c:207
        Intentional assert

E: r0/a1:  0x00000004  r1/a2:  0x000000cf  r2/a3:  0x00000000
E: r3/a4:  0x00000054 r12/ip:  0x00000000 r14/lr:  0x00002f47
E:  xpsr:  0x41000054
E: Faulting instruction address (r15/pc): 0x0000cab0
E: >>> ZEPHYR FATAL ERROR 4: Kernel panic on CPU 0
E: Fault during interrupt handling

E: Current thread: 0x20000148 (unknown)
Caught system error -- reason 4
ASSERTION FAIL [0] @ WEST_TOPDIR/zephyr/drivers/timer/sys_clock_init.c:23
E: ***** HARD FAULT *****
E:   Fault escalation (see below)
E: r0/a1:  0x00000004  r1/a2:  0x00000017  r2/a3:  0x00000000
E: r3/a4:  0x0000000f r12/ip:  0x00000000 r14/lr:  0xfffffff1
E:  xpsr:  0x4100000f
E: Faulting instruction address (r15/pc): 0x0000cab0
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Fault during interrupt handling

E: Current thread: 0x20000148 (unknown)
Caught system error -- reason 0
E: ***** USAGE FAULT *****
E:   Stack overflow (context area not valid)
E: r0/a1:  0xdde8d9e7  r1/a2:  0x5510538d  r2/a3:  0x00000d74
E: r3/a4:  0x01000000 r12/ip:  0xdfcdaf37 r14/lr:  0xe4698510
E:  xpsr:  0x53445600
E: Faulting instruction address (r15/pc): 0xf9cfef45
E: >>> ZEPHYR FATAL ERROR 2: Stack overflow on CPU 0
E: Current thread: 0x20000148 (unknown)
Caught system error -- reason 2
 PASS - test_arm_interrupt
===================================================================
START - test_arm_esf_collection
Testing ESF Reporting
E: ***** USAGE FAULT *****
E:   Attempt to execute undefined instruction
E: r0/a1:  0x00000000  r1/a2:  0x00000001  r2/a3:  0x00000002
E: r3/a4:  0x00000003 r12/ip:  0x0000000c r14/lr:  0x0000000f
E:  xpsr:  0x01000000
E: Faulting instruction address (r15/pc): 0x0000bec0
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x20000080 (unknown)
Caught system error -- reason 0
 PASS - test_arm_esf_collection
===================================================================
START - test_arm_user_interrupt
 PASS - test_arm_user_interrupt
===================================================================
Test suite arm_interrupt succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
