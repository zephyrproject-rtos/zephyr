Title: Test errno

Description:

A simple application verifies the errno value is per-thread and saved during
context switches.

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

***** BOOTING ZEPHYR OS vxxxx - BUILD: xxxxx *****
main thread, errno before starting co-op threads: abad1dea
co-op thread 0, errno before sleep: babef00d
co-op thread 1, errno before sleep: deadbeef
co-op thread 1, errno after sleep:  deadbeef
co-op thread 0, errno after sleep:  babef00d
main thread, errno after running co-op threads: abad1dea
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
