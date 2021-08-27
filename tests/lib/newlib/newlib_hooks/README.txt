Title: Newlib hook test

Description:

This test verifies the hook layer of Newlib.
It is intended to catch issues in which a library is completely absent
or non-functional, and is NOT intended to be a comprehensive test suite
of all functionality provided by the libraries.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

--------------------------------------------------------------------------------

Sample Output:

***** BOOTING ZEPHYR OS vxxxx - BUILD: xxxx *****
Running test suite test_newlib_apis
===================================================================
START - test_sbrk_error
 PASS - test_sbrk_error in 0.1 seconds
===================================================================
START - test_chk_fail
* buffer overflow detected *^M
E: EAX: 0x00110ad8, EBX: 0x00115014, ECX: 0x00133534, EDX: 0x0000001d
E: ESI: 0x00117080, EDI: 0x0010156f, EBP: 0x0012ffb8, ESP: 0x0012ffb4
E: EFLAGS: 0x00000246 CS: 0x0008 CR3: 0x0013b000
E: call trace:
E: EIP: 0x00104e2b
E:      0x00100581 (0x12ffcc)
E:      0x00104f5a (0x1015ad)
E:      0x0010157f (0x115014)
E: >>> ZEPHYR FATAL ERROR 3: Kernel oops on CPU 0
E: Current thread: 0x117080 (test_chk_fail)
Caught system error -- reason 3 1
Fatal error expected as part of test case.
 PASS - test_chk_fail in 0.3 seconds
===================================================================
START - test_newlib_api
 SKIP - test_newlib_api in 0.1 seconds
===================================================================
START - test_newlib_read
E: Page fault at address (nil) (error code 0x2)
E: Linear address not present in page tables
E: Access violation: supervisor thread not allowed to write
E: PTE: not present
E: EAX: 0x00000000, EBX: 0x00000000, ECX: 0x00118988, EDX: 0x00000000
E: ESI: 0x00000000, EDI: 0x0010156f, EBP: 0x0012ffa4, ESP: 0x0012ffa0
E: EFLAGS: 0x00000246 CS: 0x0008 CR3: 0x0013b000
E: call trace:
E: EIP: 0x00104c5f
E:      0x0010092d (0x0)
E:      0x00104f5a (0x0)
E:      0x0010157f (0x11503c)
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x117080 (test_newlib_read)
Caught system error -- reason 0 1
Fatal error expected as part of test case.
 PASS - test_newlib_read in 0.4 seconds
===================================================================
START - test_newlib_no_posix_config
 PASS - test_newlib_no_posix_config in 0.1 seconds
===================================================================
START - test_hook_install
 PASS - test_hook_install in 0.1 seconds
===================================================================
Test suite test_newlib_apis succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
