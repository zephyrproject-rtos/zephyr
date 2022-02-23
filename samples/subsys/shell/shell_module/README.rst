.. _shell_module_sample:

Shell Module Sample Application
####################################

Overview
********

This example showcases the shell module.  The shell in this sample has been
equipped with several commands for demonstration purposes.


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

Once you're connected to the UART console, you can type in `help` and press
:kbd:`Enter` to get a list of commands and descriptions of what they are
intended for.

For a quicker view of the available commands, you can also press :kbd:`Tab`,
though this will not display command descriptions.
