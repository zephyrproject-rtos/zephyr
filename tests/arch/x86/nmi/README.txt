Title: NMI registration support

Description:

This test verifies that NMI registration works as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    make run

Sample output is:

Booting from ROM..*** Booting Zephyr OS build v2.4.0-rc1-197-g77a5f92715f7  ***
Running test suite nmi
===================================================================
START - test_nmi_handler
Testing to see interrupt handler executes properly
 PASS - test_nmi_handler
===================================================================
Test suite nmi succeeded
===================================================================
PROJECT EXECUTION SUCCESSFUL
