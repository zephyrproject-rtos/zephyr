Title: Test to verify the no multithreading use-case (ARM Only)

Description:

This test verifies that a Zephyr build without multithreading
support (CONFIG_MULTITHREADING=n) works as expected. Only for
ARM Cortex-M targets. In detail the test verifies that
- system boots to main()
- PSP points to the main stack
- PSPLIM is set to the main stack base (if applicable)
- FPU state is reset (if applicable)
- Interrupts are enabled when switching to main()
- Interrupts may be registered and serviced
- Activating PendSV triggers a Reserved Exception error

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

*** Booting Zephyr OS build zephyr-v2.6.0-349-g032d10878792  ***
ARM no-multithreading test
E: ***** Reserved Exception ( -2) *****
E: r0/a1:  0x0000001b  r1/a2:  0x000044d8  r2/a3:  0xe000ed00
E: r3/a4:  0x10000000 r12/ip:  0x0000000a r14/lr:  0x00001473
E:  xpsr:  0x61000000
E: Faulting instruction address (r15/pc): 0x00001510
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: (nil) (unknown)
Caught system error -- reason 0
Available IRQ line: 25
ARM no multithreading test successful
===================================================================
PROJECT EXECUTION SUCCESSFUL
