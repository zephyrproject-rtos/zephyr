Title: Shared Floating Point Support

Description:

This test uses the background task and a fiber to independently load and
store floating point registers and check for corruption. This tests the
ability of contexts to safely share floating point hardware resources, even
when switching occurs preemptively.

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

Floating point sharing tests started
===================================================================
Load and store OK after 100 (high) + 83270 (low) tests
Load and store OK after 200 (high) + 164234 (low) tests
Load and store OK after 300 (high) + 245956 (low) tests
Load and store OK after 400 (high) + 330408 (low) tests
Load and store OK after 500 (high) + 411981 (low) tests
===================================================================
PROJECT EXECUTION SUCCESSFUL
