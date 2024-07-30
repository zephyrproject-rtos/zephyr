Title: Test to verify the behavior of HardFault (ARM Only)

Description:

This test verifies the Cortex-M HardFault escalation. Only for
ARM Cortex-M targets.

---------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed on QEMU as
follows:

    make run

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

*** Booting Zephyr OS build zephyr-v2.6.0-482-g9daa69b212cd  ***
Running test suite arm_hardfault_validation
===================================================================
START - test_arm_hardfault
E: r0/a1:  0x00000004  r1/a2:  0x00000000  r2/a3:  0x00000004
E: r3/a4:  0x20000000 r12/ip:  0x00000000 r14/lr:  0x000029fb
E:  xpsr:  0x41000000
E: Faulting instruction address (r15/pc): 0x0000079e
E: >>> ZEPHYR FATAL ERROR 4: Kernel panic on CPU 0
E: Current thread: 0x20000070 (test_arm_hardfault)
Caught system error -- reason 4
ASSERTION FAIL [0] @ ../src/arm_hardfault.c:42
        Assert occurring inside kernel panic
E: ***** HARD FAULT *****
E:   Fault escalation (see below)
E: ARCH_EXCEPT with reason 4

E: r0/a1:  0x00000004  r1/a2:  0x0000002a  r2/a3:  0x00000001
E: r3/a4:  0x000016f9 r12/ip:  0xa0000000 r14/lr:  0x0000075f
E:  xpsr:  0x4100000b
E: Faulting instruction address (r15/pc): 0x00005d1e
E: >>> ZEPHYR FATAL ERROR 4: Kernel panic on CPU 0
E: Fault during interrupt handling

E: Current thread: 0x20000070 (test_arm_hardfault)
Caught system error -- reason 4
 PASS - test_arm_hardfault in 0.79 seconds
===================================================================
Test suite arm_hardfault_validation succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
