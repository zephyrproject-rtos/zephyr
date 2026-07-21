Title: Installation of ISRs Directly in the Vector Table (ARM Only)

Description:

Verify a project can install ISRs directly in the vector table. Only for
ARM Cortex-M targets.

---------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed on QEMU as
follows:

    make run

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

Running TESTSUITE vector_table
===================================================================
START - test_arm_irq_vector_table
Test Cortex-M IRQs installed directly in the vector table
 PASS - test_arm_irq_vector_table in 0.007 seconds
===================================================================
TESTSUITE vector_table succeeded

------ TESTSUITE SUMMARY START ------

SUITE PASS - 100.00% [vector_table]: pass = 1, fail = 0, skip = 0, total = 1 duration = 0.007 seconds
 - PASS - [vector_table.test_arm_irq_vector_table] duration = 0.007 seconds

------ TESTSUITE SUMMARY END ------

===================================================================
PROJECT EXECUTION SUCCESSFUL
