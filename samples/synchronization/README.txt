Title: Synchronization

Description:

A simple application that demonstates basic sanity of the kernel.
Two threads (A and B) take turns printing a greeting message to the console,
and use sleep requests and semaphores to control the rate at which messages
are generated. This demonstrates that kernel scheduling, communication,
and timing are operating correctly.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
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

threadA: Hello World!
threadB: Hello World!
threadA: Hello World!
threadB: Hello World!
threadA: Hello World!
threadB: Hello World!
threadA: Hello World!
threadB: Hello World!
threadA: Hello World!
threadB: Hello World!

<repeats endlessly>
