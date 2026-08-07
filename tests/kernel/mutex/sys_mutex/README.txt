Title: System Mutex APIs

Description:

This test verifies that the sys_mutex APIs operate as expected, including
when accessed from user mode on platforms that support userspace. The
mutex_complex suite runs two cases:

1. test_mutex_multithread_competition
   - Multiple threads contend for a sys_mutex and the priority-inheritance
     and locking behavior is checked, including recursive locking.

2. test_supervisor_access
   - A supervisor thread accesses the mutex directly to confirm the
     supervisor path behaves the same as the user-mode path.

The kernel.mutex.system test builds with userspace enabled, while
kernel.mutex.system.nouser builds with CONFIG_TEST_USERSPACE=n.

---------------------------------------------------------------------------

Building and Running:

Build and run with twister, for example on QEMU:

    twister -p qemu_x86 -T tests/kernel/mutex/sys_mutex

Or build and run a single platform directly with west:

    west build -b qemu_x86 tests/kernel/mutex/sys_mutex
    west build -t run

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE mutex_complex
===================================================================
START - test_mutex_multithread_competition
 PASS - test_mutex_multithread_competition
===================================================================
START - test_supervisor_access
 PASS - test_supervisor_access
===================================================================
TESTSUITE mutex_complex succeeded
