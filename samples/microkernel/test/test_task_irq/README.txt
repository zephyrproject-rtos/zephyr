Title: task level interrupt handling test

Description:

This test exercises the APIs of the task level interrupt handling feature.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

--------------------------------------------------------------------------------

Sample Output:

Starting task level interrupt handling tests
===================================================================
IRQ object 0 using IRQ8 allocated
IRQ object 1 using IRQ14 allocated

IRQ object 2 using IRQ32 allocated
IRQ object 3 using IRQ34 allocated

Generating interrupts for all allocated IRQ objects...
Received event for IRQ object 0
Received event for IRQ object 1
Received event for IRQ object 2
Received event for IRQ object 3

Attempt to allocate an IRQ object that
is already allocated by another task...
Re-allocation of IRQ object 3 prevented

Attempt to allocate an IRQ that
is already allocated by another task...
Re-allocation of IRQ34 prevented

Attempt to free an IRQ object...
IRQ object 2 freed
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
