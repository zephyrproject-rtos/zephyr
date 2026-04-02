Title: A common fatal error and assert fail handler

Description:

These two common handlers are developed in order to reduce the redundancy
code writing for fatal and assert handler for error case testing. They can
be used both in kernel and userspace, and they are also SMP safe.


Why doing this
==============

When writing error testing case (or we call it negative test case), we might
have to write self-defined k_sys_fatal_handler or post_assert_handler to deal
with the errors we caught, in order to make the test continue. This means much
identical code would be written. So we try to move the error handler definition
into a common part and let other test suites or applications reuse it, instead
of defining their own.

And when error happens, we sometimes need a special action to make our testing
program return back to normal, such as releasing some resource hold before the
error happened. This is why we add a hook on it, in order to achieve that goal.


How to use it in you app
========================

(a) Usage for dealing with fatal error:

Step1: Add CONFIG_ZTEST_FATAL_HOOK=y into prj.conf

Step2: Include <ztest_fatal_hook.h> in C source file.

Step3: (optional) Define a hook function call ztest_post_fatal_error_hook().

Step4: Call ztest_set_fault_valid(true) before where your target function
       call.


(b) Usage for dealing with assert fail:

Step1: Add CONFIG_ZTEST_ASSERT_HOOK=y into prj.conf

Step2: Include <zephyr/ztest_error_hook.h> in your C code.

Step3: (optional) Define a hook function call ztest_post_assert_fail_hook().

Step4: call ztest_set_assert_valid(true) before where your target function
       call.


You can choose to use one or both of them, depending on your needs.
If you use none of them, you can still define your own fatal or assert handler
as usual.


Test example in this test set
=============================

This test verifies if the common fatal error and assert fail handler works.
If the expected error was caught, the test case will pass.

test_catch_assert_fail
  - To call a function then giving the condition to trigger the assert fail,
    then catch it by the assert handler.

test_catch_fatal_error
  - start a thread to test triggering a null address dereferencing, then catch
    the (expected) fatal error.
  - start a thread to test triggering an illegal instruction, then catch
    the (expected) fatal error.
  - start a thread to test triggering a divide-by-zero error, then catch
    the (expected) fatal error.
  - start a thread to call k_oops() then catch the (expected) fatal error.
  - start a thread to call k_panel() then catch the (expected) fatal error.

test_catch_assert_in_isr
  - start a thread to enter ISR context by calling irq_offload(), then trigger
    an assert fail then catch it.

test_catch_z_oops
  - Pass illegal address by syscall, then inside the syscall handler, the
    K_OOPS macro will trigger a fatal error that will get caught (as expected).



Limitation of this usage
========================

Trigger a fatal error in ISR context, that will cause problem due to
the interrupt stack is already abnormal when we want to continue other
test case, we do not recover it so far.


---------------------------------------------------------------------------

Sample Output:

Running test suite error_hook_tests
===================================================================
START - test_catch_assert_fail
ASSERTION FAIL [a != ((void *)0)] @ WEST_TOPDIR/zephyr/tests/ztest/error_hook/src/main.c:41
        parameter a should not be NULL!
Caught assert failed
Assert error expected as part of test case.
 PASS - test_catch_assert_fail
===================================================================
START - test_catch_fatal_error
case type is 0
E: Page fault at address (nil) (error code 0x4)
E: Linear address not present in page tables
E: Access violation: user thread not allowed to read
E: PTE: not present
E: EAX: 0x00000000, EBX: 0x00000000, ECX: 0x00000000, EDX: 0x0010fe51
E: ESI: 0x00000000, EDI: 0x0012dfe8, EBP: 0x0012dfcc, ESP: 0x0012dfc4
E: EFLAGS: 0x00000246 CS: 0x002b CR3: 0x001142c0
E: call trace:
E: EIP: 0x00100439
E:      0x001010ea (0x113068)
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x114000 (unknown)
Caught system error -- reason 0 1
Fatal error expected as part of test case.
case type is 1
E: Page fault at address 0x12dfc4 (error code 0x15)
E: Access violation: user thread not allowed to execute
E: PTE: 0x12d000 -> 0x000000000012d000: RW US A D XD
E: EAX: 0x0012dfc4, EBX: 0x00000001, ECX: 0x00000001, EDX: 0x0010fe51
E: ESI: 0x00000000, EDI: 0x0012dfe8, EBP: 0x0012dfcc, ESP: 0x0012dfc0
E: EFLAGS: 0x00000246 CS: 0x002b CR3: 0x001142c0
E: call trace:
E: EIP: 0x0012dfc4
E:      0x001010ea (0x113068)
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x114000 (unknown)
Caught system error -- reason 0 1
Fatal error expected as part of test case.
case type is 2
E: Invalid opcode
E: EAX: 0x00000000, EBX: 0x00000002, ECX: 0x00000002, EDX: 0x0010fe51
E: ESI: 0x00000000, EDI: 0x0012dfe8, EBP: 0x0012dfcc, ESP: 0x0012dfc4
E: EFLAGS: 0x00000246 CS: 0x002b CR3: 0x001142c0
E: call trace:
E: EIP: 0x00100451
E:      0x001010ea (0x113068)
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x114000 (unknown)
Caught system error -- reason 0 1
Fatal error expected as part of test case.
case type is 3
E: EAX: 0x00000000, EBX: 0x00000003, ECX: 0x00000003, EDX: 0x0010fe51
E: ESI: 0x00000000, EDI: 0x0012dfe8, EBP: 0x0012dfcc, ESP: 0x0012dfc0
E: EFLAGS: 0x00000246 CS: 0x002b CR3: 0x001142c0
E: call trace:
E: EIP: 0x0010045c
E:      0x001010ea (0x113068)
E: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
E: Current thread: 0x114000 (unknown)
Caught system error -- reason 3 1
Fatal error expected as part of test case.
case type is 4
E: EAX: 0x00000000, EBX: 0x00000004, ECX: 0x00000004, EDX: 0x0010fe51
E: ESI: 0x00000000, EDI: 0x0012dfe8, EBP: 0x0012dfcc, ESP: 0x0012dfc0
E: EFLAGS: 0x00000246 CS: 0x002b CR3: 0x001142c0
E: call trace:
E: EIP: 0x00100465
E:      0x001010ea (0x113068)
E: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
E: Current thread: 0x114000 (unknown)
Caught system error -- reason 3 1
Fatal error expected as part of test case.
 PASS - test_catch_fatal_error
===================================================================
START - test_catch_assert_in_isr
ASSERTION FAIL [a != ((void *)0)] @ WEST_TOPDIR/zephyr/tests/ztest/error_hook/src/main.c:41
        parameter a should not be NULL!
Caught assert failed
Assert error expected as part of test case.
 PASS - test_catch_assert_in_isr
===================================================================
START - test_catch_z_oops
E: EAX: 0x00000000, EBX: 0x00000000, ECX: 0x00000000, EDX: 0x00000000
E: ESI: 0x00000000, EDI: 0x00000000, EBP: 0x00000000, ESP: 0x00000000
E: EFLAGS: 0x001003c4 CS: 0x0511 CR3: 0x00115740
E: call trace:
E: EIP: 0x0010ff9e
E: NULL base ptr
E: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
E: Current thread: 0x114120 (unknown)
Caught system error -- reason 3 1
Fatal error expected as part of test case.
 PASS - test_catch_z_oops
===================================================================
Test suite error_hook_tests succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
