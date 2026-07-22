.. zephyr:code-sample:: watchpoint
   :name: Memory watchpoint
   :relevant-api: watchpoint_apis

   Use a hardware watchpoint to identify an instruction that writes to memory.

Overview
********

This sample arms a write watchpoint on a global variable, modifies the variable,
and captures the triggering program counter in the watchpoint callback. The
callback only stores event data. Output is produced later from thread context.

Requirements
************

The target must provide a hardware watchpoint backend. The sample is
automatically tested on the ``qemu_riscv32`` and ``qemu_riscv64`` boards.

Building and running
********************

Build and run the sample with:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/debug/watchpoint
   :board: qemu_riscv64
   :goals: run
   :compact:

Sample output
*************

The reported timing depends on the hardware implementation:

.. code-block:: console

   Watchpoint hit: pc=0x80001234 timing=before rearm_required=yes
   Watchpoint sample complete
