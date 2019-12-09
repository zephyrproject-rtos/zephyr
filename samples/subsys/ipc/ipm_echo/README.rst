.. _ipm_echo:

IPM echo
########

Overview
********

IPM echo application reads mailbox and writes back.

Building and Running
********************

The project can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipm_echo
   :host-os: unix
   :board: up_squared_adsp
   :goals: build
   :compact:

Follow the instructions in the :ref:`Up_Squared_Audio_DSP` board documentation
for how to load the Zephyr binary to the Xtensa core and execute it.

Sample Output
=============

After loading and executing Zephyr application to Xtensa core we can get logs:

.. code-block:: console

   $ sudo boards/xtensa/up_squared_adsp/tools/dump_trace.py
   <dbg> app.main: Starting IPM echo application
   <dbg> app.main: IPM initialized

On a Linux side we are using devmem2 tool to work with mailboxes through
memory mapped registers.

Clear Done for the mailbox with:

.. code-block:: console

   $ sudo devmem2 0x91200040 w 0x80000000

Write some data to the mailbox with:

.. code-block:: console

   $ sudo devmem2 0x912a0000 w 0xdeadface

Trigger write and set mailbox id with:

.. code-block:: console

   $ sudo devmem2 0x91200048 w 0x800000cd

We can now verify that data is written back with:

.. code-block:: console

   $ sudo devmem2 0x91281000
   /dev/mem opened.
   Memory mapped at address 0x7ff82f15d000.
   Value at address 0x91281000 (0x7ff82f15d000): 0xDEADFACE

And the logs show that the callback got called:

.. code-block:: console

   $ sudo boards/xtensa/up_squared_adsp/tools/dump_trace.py
   ...
   <dbg> app.ipm_callback: id 0xcd
   <dbg> app: data
   ce fa ad de                                      |....
