Title: Test to verify the behavior of CONFIG_RUNTIME_NMI at runtime (ARM Only)

Description:

This test verifies the behavior of CONFIG_RUNTIME_NMI at runtime. It is only
for ARM Cortex-M targets.

The arm_runtime_nmi_fn suite installs an NMI handler at run time using
z_arm_nmi_set_handler(), pends the NMI by setting the NMIPENDSET bit in the
SCB ICSR, and verifies that the registered handler fires.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p mps2/an385 -T tests/arch/arm/arm_runtime_nmi

Or build and run a single platform directly with west:

    west build -b mps2/an385 tests/arch/arm/arm_runtime_nmi
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE arm_runtime_nmi_fn
===================================================================
START - test_arm_runtime_nmi
Trigger NMI in 2s: 0 s
Trigger NMI in 2s: 1 s
NMI triggered (test_handler_isr)!
 PASS - test_arm_runtime_nmi
===================================================================
TESTSUITE arm_runtime_nmi_fn succeeded
