.. _spike_riscv64:

RISCV64 Emulation (Spike)
#############

Overview
********
The RISCV64 Spike board configuration is used to emulate the RISCV64 architecture.

Programming and Debugging
*************************
Applications for the ``spike_riscv64`` board configuration can be built and run in
the usual way for emulated boards (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========
While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications in the Spike
emulated environment. For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: spike_riscv64
   :goals: build

This will build an image with the synchronization sample app.
You can boot it using SPIKE with the following command line:

.. code-block:: console

   isnogud% spike --isa=rv64imac build/zephyr/zephyr.elf
   warning: tohost and fromhost symbols not in ELF; can't communicate with target
   *** Booting Zephyr OS build v3.6.0-3379-gea47ef0ac0ef ***
   thread_a: Hello World from cpu 0 on spike_riscv64!
   thread_b: Hello World from cpu 0 on spike_riscv64!
   thread_a: Hello World from cpu 0 on spike_riscv64!
   thread_b: Hello World from cpu 0 on spike_riscv64!
   thread_a: Hello World from cpu 0 on spike_riscv64!
   thread_b: Hello World from cpu 0 on spike_riscv64!
   thread_a: Hello World from cpu 0 on spike_riscv64!
   thread_b: Hello World from cpu 0 on spike_riscv64!
   thread_a: Hello World from cpu 0 on spike_riscv64!

Exit SPIKE by pressing :kbd:`CTRL+C` :kbd:`q`.

Debugging
=========

If you want to use gdb for debugging you need to add following options to spike:

.. code-block:: console

   isnogud% spike --isa=rv64imac -m0x80000000:0x10000000 --rbb-port=9824 build/zephyr/zephyr.elf
   Listening for remote bitbang connection on port 9824.
   warning: tohost and fromhost symbols not in ELF; can't communicate with target
   *** Booting Zephyr OS build v3.6.0-3379-gea47ef0ac0ef ***
   thread_a: Hello World from cpu 0 on spike_riscv64!
   thread_b: Hello World from cpu 0 on spike_riscv64!

Then attach using openocd (an usable openocd configuration is available in the spike README on
github page):

.. code-block:: console

   isnogud% ./riscv-openocd/src/openocd -f ~/spike.cfg
   Open On-Chip Debugger 0.12.0+dev-03746-g16db1b77f (2024-04-26-11:14)
   Licensed under GNU GPL v2
   For bug reports, read
      http://openocd.org/doc/doxygen/bugs.html
   Info : auto-selecting first available session transport "jtag". To override use 'transport select <transport>'.
   Info : Initializing remote_bitbang driver
   Info : Connecting to localhost:9824
   Info : remote_bitbang driver initialized
   Info : Note: The adapter "remote_bitbang" doesn't support configurable speed
   Info : JTAG tap: riscv.cpu tap/device found: 0xdeadbeef (mfg: 0x777 (Fabric of Truth Inc), part: 0xeadb, ver: 0xd)
   Info : [riscv.cpu] datacount=2 progbufsize=2
   Info : [riscv.cpu] Examined RISC-V core
   Info : [riscv.cpu]  XLEN=64, misa=0x8000000000141105
   [riscv.cpu] Target successfully examined.
   Info : [riscv.cpu] Examination succeed
   Info : starting gdb server for riscv.cpu on 3333
   Info : Listening on port 3333 for gdb connections
   riscv.cpu halted due to debug-request.
   Info : Listening on port 6666 for tcl connections
   Info : Listening on port 4444 for telnet connections

And finally connect using gdb:

.. code-block:: console

   (gdb) target extended-remote :3333
   Remote debugging using :3333
   arch_irq_unlock (key=8) at /home/edwarf/git/riscv_spike/zephyr-project/zephyr/include/zephyr/arch/riscv/arch.h:259
   259		__asm__ volatile ("csrs mstatus, %0"

Hint: If you receive errors, that gdb/openocd cannot write into memory, this is due to enabled PMP.
Either compile Zephyr with PMP disabled, use hardware breakpoints or add ``--dm-sba=64`` to the spike
commandline, which allows openocd to access the system bus and bypass PMP.

References
**********

.. Spike github:
   https://github.com/riscv-software-src/riscv-isa-sim
