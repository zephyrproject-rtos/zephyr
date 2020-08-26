.. _nsim:

DesignWare(R) ARC(R) Emulation (nsim)
#####################################

Overview
********

This board configuration will use `Designware ARC nSIM`_ to emulate a virtual
ARC EM or ARC HS based board including the following features:

* ARC EM or ARC HS processor
* ARC internal timer
* a virtual console (ns16550 based UART model)

There are four supported board sub-configurations:

* ``nsim_em`` which includes normal ARC EM features and ARC MPUv2
* ``nsim_sem`` which includes secure ARC EM features and ARC MPUv3
* ``nsim_hs`` which includes base ARC HS features, i.e. w/o PMU and MMU
* ``nsim_hs_smp`` which includes base ARC HS features in multi-core cluster, still w/o PMU and MMU

For detailed arc features, please refer to
:zephyr_file:`boards/arc/nsim/support/nsim_em.props`,
:zephyr_file:`boards/arc/nsim/support/nsim_sem.props`,
:zephyr_file:`boards/arc/nsim/support/nsim_hs.props` and
:zephyr_file:`boards/arc/nsim/support/mdb_hs_smp.args`


Hardware
********
Supported Features
==================

The following hardware features are supported:

+-----------+------------+-----+-------+-----+-----------------------+
| Interface | Controller | EM  | SEM   | HS  | Driver/Component      |
+===========+============+=====+=======+=====+=======================+
| INT       | on-chip    | Y   | Y     | Y   | interrupt_controller  |
+-----------+------------+-----+-------+-----+-----------------------+
| UART      | ns16550    | Y   | Y     | Y   | serial port           |
+-----------+------------+-----+-------+-----+-----------------------+
| TIMER     | on-chip    | Y   | Y     | Y   | system clock          |
+-----------+------------+-----+-------+-----+-----------------------+


Programming and Debugging
*************************

Required Hardware and Software
==============================

To use Zephyr RTOS applications on this board, `Designware ARC nSIM`_ or
`Designware ARC nSIM Lite`_ is required.

Building Sample Applications
==============================

Use this configuration to run basic Zephyr applications and kernel tests in
nSIM, for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: nsim_em
   :goals: flash

This will build an image with the synchronization sample app, boot it using
nsim, and display the following console output:

.. code-block:: console

        ***** BOOTING ZEPHYR OS v1.12 - BUILD: July 6 2018 15:17:26 *****
        threadA: Hello World from arc!
        threadB: Hello World from arc!
        threadA: Hello World from arc!
        threadB: Hello World from arc!


.. note::
   To exit the simulator, use Ctrl+], then Ctrl+c

Debugging
=========

.. note::
   The normal ``make debug`` command won't work for debugging
   applications using nsim because both the nsim simulator and the
   gdb debugger use the console for output. You need to use separate
   terminal windows for each tool to avoid intermixing their output.

After building your application, cd to the build folder and open two
terminal windows. In terminal one, use nsim to start a GDB server
and wait for a remote connection:

.. code-block:: console

   # for ninja build system:
   ninja debugserver
   # for make build system:
   make debugserver

In terminal two, connect to the GDB server using :file:`arc-elf32-gdb`.
This command loads the symbol table from the elf binary file, for example
the :file:`./zephyr/zephyr.elf` file:

.. code-block:: console

   ..../path/to/arc-elf32-gdb zephyr/zephyr.elf
   (gdb) target remote : 3333
   (gdb) load

Now the debug environment has been set up, you can debug the application with gdb commands.


References
**********

.. _Designware ARC nSIM: https://www.synopsys.com/dw/ipdir.php?ds=sim_nsim
.. _Designware ARC nSIM Lite: https://www.synopsys.com/cgi-bin/dwarcnsim/req1.cgi
