Title: Test to verify the no multithreading use-case (ARM Only)

Description:

This test verifies that a Zephyr build without multithreading support
(CONFIG_MULTITHREADING=n) works as expected. It is only for ARM Cortex-M
targets.

Because multithreading is disabled there is no threaded Ztest suite; the
test runs from test_main() and checks that the system boots to main() with
PSP and PSPLIM pointing into the main stack, that the FPU state is reset
(when CONFIG_FPU is enabled) and interrupts are enabled on entry to main(),
that activating PendSV escalates to a Reserved Exception
(K_ERR_CPU_EXCEPTION) caught by the fatal-error handler, and that an
interrupt can be dynamically registered, enabled, pended and serviced on a
free NVIC line.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_cortex_m0 -T tests/arch/arm/arm_no_multithreading

Or build and run a single platform directly with west:

    west build -b qemu_cortex_m0 tests/arch/arm/arm_no_multithreading
    west build -t run

---------------------------------------------------------------------------

Sample Output:

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
