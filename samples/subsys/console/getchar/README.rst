.. _console_getchar_sample:

console_getchar() Sample Application
####################################

Overview
********

This example shows how to use :c:func:`console_getchar` function.
Similar to the well-known ANSI C getchar() function,
:c:func:`console_getchar` either returns the next available input
character or blocks waiting for one. Using this function, it should be
fairly easy to port existing ANSI C, POSIX, or Linux applications which
process console input character by character. The sample also allows to
see key/character codes as returned by the function.

If you are interested in line by line console input, see
:ref:`console_getline_sample`.


Requirements
************

UART console is required to run this simple.


Building and Running
********************

The easiest way to run this sample is using QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/console/getchar
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Now start pressing keys on a keyboard, and they will be printed both as
hex values and in character form. Be sure to press Enter, Up/Down, etc.
key to check what control characters are produced for them.
Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
