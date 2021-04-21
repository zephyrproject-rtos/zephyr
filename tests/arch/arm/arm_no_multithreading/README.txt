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
- Interrupts may be registerd and serviced

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

***  Booting Zephyr OS build zephyr-v2.3.0-1561-ge877a95387ae  ***
ARM no-multithreading test
Available IRQ line: 25
ARM no multithreading test successful
===================================================================
PROJECT EXECUTION SUCCESSFUL
