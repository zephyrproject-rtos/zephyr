Title: Test to verify the behavior of CONFIG_RUNTIME_NMI at runtime (ARM Only)

Description:

This test verifies the behavior of CONFIG_RUNTIME_NMI at runtime. Only for
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

Trigger NMI in 10s: 0 s
Trigger NMI in 10s: 1 s
Trigger NMI in 10s: 2 s
Trigger NMI in 10s: 3 s
Trigger NMI in 10s: 4 s
Trigger NMI in 10s: 5 s
Trigger NMI in 10s: 6 s
Trigger NMI in 10s: 7 s
Trigger NMI in 10s: 8 s
Trigger NMI in 10s: 9 s
NMI received (test_handler_isr)! Rebooting...
...
