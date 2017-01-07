Title: Test multilib

Description:

Some architectures support different ISA variants, each backed a different
multilib in a same compiler. Selecting wrong multilib may have adverse
effects on performance, or sometime produce broken executables altogether
(for example, ARM Cortex-M requires thumb2 multilib and will be broken with
default ("arm") multilib or "thumb" multlib). This app is a smoke-test
for selecting non-wrong multilib - it uses operation(s) which guaranteedly
will call support routine(s) in libgcc and checks for expected result.

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

Sample output:

===================================================================
PASS - main.
===================================================================
PROJECT EXECUTION SUCCESSFUL

Sample failure output ("thumb" used on Cortex-M instead of "thumb2"):

***** USAGE FAULT *****
  Executing thread ID (thread): 0x200002a0
  Faulting instruction address:  0x00000000
  Illegal use of the EPSR
Fatal fault in essential task ! Spinning...
