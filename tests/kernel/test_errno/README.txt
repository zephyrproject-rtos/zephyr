Title: Test errno

Description:

A simple application verifies the errno value is per-thread and saved during
context switches.

--------------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make qemu

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

task, errno before starting fibers: abad1dea
fiber 0, errno before sleep: babef00d
fiber 1, errno before sleep: deadbeef
fiber 1, errno after sleep:  deadbeef
fiber 0, errno after sleep:  babef00d
task, errno after running fibers:   abad1dea
===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL
