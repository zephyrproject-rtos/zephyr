.. zephyr:code-sample:: console_getline
   :name: console_getline()

   Use console_getline() to read an input line from the console.

Overview
********

This example shows how to use :c:func:`console_getline` function.
Similar to the well-known ANSI C gets() and fgets() functions,
:c:func:`console_getline` either returns the next available input
line or blocks waiting for one. Using this function, it should be fairly
easy to port existing ANSI C, POSIX, or Linux applications which process
console input line by line. The sample also allows to see details of how
a line is returned by the function.

If you are interested in character by character console input, see
:zephyr:code-sample:`console_getchar`.


Requirements
************

UART console is required to run this sample.


Building and Running
********************

The easiest way to run this sample is using QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/console/getline
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Now start pressing keys on a keyboard, followed by Enter. The input line
will be printed back, with a hex code of the last character, to show that
line does not include any special "end of line" characters (like LF, CR,
etc.)
Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
