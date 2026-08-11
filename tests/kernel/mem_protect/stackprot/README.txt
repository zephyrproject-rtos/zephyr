Title: Stack Protection Support

Description:

This test verifies that stack canaries and stack guard pages operate as
expected. It runs two Ztest suites:

1. stackprot
   - test_stackprot (a user-mode case) overflows a thread stack and
     confirms the stack-canary check detects the corruption and aborts
     the offending thread while the rest of the system keeps running.
   - test_create_alt_thread and test_canary_value exercise creating the
     alternate thread and inspecting the canary value.

2. stackprot_mapped_stack
   - test_guard_page_front and test_guard_page_rear (plus their _user
     variants) confirm that the guard pages placed in front of and behind
     a mapped stack fault on overflow.

Because the test deliberately triggers faults, it is run with
ignore_faults enabled in twister.

The kernel.memory_protection.stackprot_tls variant additionally places the
canary in thread-local storage (CONFIG_STACK_CANARIES_TLS=y) on
architectures that support it.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/kernel/mem_protect/stackprot

Or build and run a single platform directly with west:

    west build -b qemu_x86 tests/kernel/mem_protect/stackprot
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE stackprot
===================================================================
START - test_canary_value
 PASS - test_canary_value
===================================================================
START - test_create_alt_thread
 PASS - test_create_alt_thread
===================================================================
START - test_stackprot
alternate_thread: Input string is too long and stack overflowed!
*** Stack Check Fail! ***
 PASS - test_stackprot
===================================================================
TESTSUITE stackprot succeeded

Running TESTSUITE stackprot_mapped_stack
===================================================================
START - test_guard_page_front
 PASS - test_guard_page_front
===================================================================
START - test_guard_page_rear
 PASS - test_guard_page_rear
===================================================================
TESTSUITE stackprot_mapped_stack succeeded
