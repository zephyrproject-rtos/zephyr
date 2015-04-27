Title: Hello

Description:

A simple application that demonstrates basic sanity of the VxMicro nanokernel.
The background task and a fiber take turns printing a greeting message to the
console, and use timers and semaphores to control the rate at which messages
are generated. This demonstrates that nanokernel scheduling, communication,
and timing are operating correctly.

--------------------------------------------------------------------------------

Building and Running Project:

This nanokernel project outputs to the console.  It can be built and executed
on QEMU as follows:

    make pristine
    make nanokernel.qemu

--------------------------------------------------------------------------------

Sample Output:

main: Hello World!
fiberEntry: Hello World!
main: Hello World!
fiberEntry: Hello World!
main: Hello World!
fiberEntry: Hello World!
main: Hello World!
fiberEntry: Hello World!
main: Hello World!
fiberEntry: Hello World!

<repeats endlessly>
