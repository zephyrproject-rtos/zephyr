Title: arm_m3-irq_vector_table

Description:

Verify a project can install ISRs directly in the vector table. Only for
ARM Cortex-M3/4 targets.

---------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make nanokernel.qemu

---------------------------------------------------------------------------

Sample Output:

tc_start() - Test Cortex-M3 IRQ installed directly in vector table
isr0 ran!
isr1 ran!
isr2 ran!
PASS - main.
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
