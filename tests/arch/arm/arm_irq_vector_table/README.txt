Title: Installation of ISRs Directly in the Vector Table (ARM Only)

Description:

Verify a project can install ISRs directly in the vector table. Only for
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

tc_start() - Test Cortex-M3 IRQ installed directly in vector table
isr0 ran!
isr1 ran!
isr2 ran!
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
