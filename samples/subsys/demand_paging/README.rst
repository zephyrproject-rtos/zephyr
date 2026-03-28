.. zephyr:code-sample:: demand-paging
   :name: Demand paging
   :relevant-api: mem-demand-paging

   Leverage demand paging to deal with code bigger than available RAM.

Overview
********

This sample demonstrates how demand paging can be leveraged to deal with
firmware bigger than the available RAM if XIP is not possible. Select code
can be tagged to be loaded into memory on demand, and also be automatically
evicted for more code to be executed when memory is exhausted.

Requirements
************

This demo requires the presence of a supported hardware MMU and a backing
store implementation with access to the compiled Zephyr binary.
For demonstration purposes, the ondemand_semihost backing store is used on
a QEMU ARM64 target with a hardcoded small RAM configuration.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/demand_paging
   :host-os: unix
   :board: qemu_cortex_a53
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build v3.7.0-2108-g5975c3785356 ***
    Calling huge body of code that doesn't fit in memory
    free memory pages: from 37 to 0, 987 page faults
    Calling it a second time
    free memory pages: from 0 to 0, 987 page faults
    Done.

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

To actually view the underlying demand paging subsystem at work, debug
prints can be enabled using :kconfig:option:`CONFIG_LOG` and
:kconfig:option:`CONFIG_KERNEL_LOG_LEVEL_DBG` in the config file.
