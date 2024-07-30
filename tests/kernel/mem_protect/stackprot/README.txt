Title: Stack Protection Support

Description:

This test verifies that stack canaries operate as expected.

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
tc_start() - Test Stack Protection Canary

Starts main
Starts alternate_thread
alternate_thread: Input string is too long and stack overflowed!

***** Stack Check Fail! *****
Current thread ID = 0x00103180
Faulting segment:address = 0xdead:0xdeaddead
eax: 0xdeaddead, ebx: 0xdeaddead, ecx: 0xdeaddead, edx: 0xdeaddead
esi: 0xdeaddead, edi: 0xdeaddead, ebp: 0deaddead, esp: 0xdeaddead
eflags: 0xdeaddead
Fatal fault in thread 0x00103180! Aborting.
main: Stack ok
main: Stack ok
main: Stack ok
main: Stack ok
main: Stack ok
main: Stack ok
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
