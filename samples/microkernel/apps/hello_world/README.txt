Title: Hello

Description:

A simple application that demonstates basic sanity of the VxMicro microkernel.
Two tasks (A and B) take turns printing a greeting message to the console,
and use sleep requests and semaphores to control the rate at which messages
are generated. This demonstrates that microkernel scheduling, communication,
and timing are operating correctly.

--------------------------------------------------------------------------------

Building and Running Project:

This microkernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make microkernel.qemu

If executing on Simics, substitute 'simics' for 'qemu' in the command line.

--------------------------------------------------------------------------------

Sample Output:

taskA: Hello World!
taskB: Hello World!
taskA: Hello World!
taskB: Hello World!
taskA: Hello World!
taskB: Hello World!
taskA: Hello World!
taskB: Hello World!
taskA: Hello World!
taskB: Hello World!

<repeats endlessly>
