.. _echo_sample:

echo Sample Application
#######################

Overview
********

This example shows how to use the :c:func:`console_getchar` and
:c:func:`console_putchar` functions together to print out to standard output the
characters which are read from standard input.


Requirements
************

A UART console is required to run this sample.


Building and Running
********************

The easiest way to run this sample is using QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/console/echo
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

When running this sample, the user shall expect to see printed out in the
console what they are entering via the keyboard.
