Title: test_fp_sharing

Description:

This test uses the background task and a fiber to independently load and
store floating point registers and check for corruption. This tests the
ability of contexts to safely share floating point hardware resources, even
when switching occurs preemptively.

--------------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make nanokernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

--------------------------------------------------------------------------------

Sample Output:

Floating point sharing tests started
===================================================================
Load and store OK after 100 (high) + 83270 (low) tests
Load and store OK after 200 (high) + 164234 (low) tests
Load and store OK after 300 (high) + 245956 (low) tests
Load and store OK after 400 (high) + 330408 (low) tests
Load and store OK after 500 (high) + 411981 (low) tests
Load and store OK after 600 (high) + 495450 (low) tests
Load and store OK after 700 (high) + 579436 (low) tests
Load and store OK after 800 (high) + 663078 (low) tests
Load and store OK after 900 (high) + 746627 (low) tests
Load and store OK after 1000 (high) + 826008 (low) tests
===================================================================
VXMICRO PROJECT EXECUTION SUCCESSFUL
