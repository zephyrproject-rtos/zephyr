.. _hello_world:

Demand Paging
#############

Overview
********

This sample demonstrates how demand paging can be leveraged to deal with
firmware bigger than the available RAM if XIP is not possible. Select code
can be tagged to be loaded into memory on demand, and also be automatically
evicted for more code to be executed when memory is exhausted.

Requirements
************

This demo requires the presence of a MMU in hardware that Zephyr supports,
and a backing store implementation with access to the compiled Zephyr binary.
For demonstration purposes, the ondemand_semihost backing store on a QEMU
ARM64 target is used to that effect with a configured small amount of RAM.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/demand_paging
   :host-os: unix
   :board: qemu_cortex_a53
   :goals: run
   :compact:

.. warning::
   The code relies on multiple levels of macro expansions to grow the code
   size. This makes compilation very slow.

Sample Output
=============

.. code-block:: console

    Calling huge body of code that doesn't fit in memory
    free memory pages: from 100 to 0, 983 page faults
    Calling it a second time
    free memory pages: from 0 to 0, 983 page faults
    Done.

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
