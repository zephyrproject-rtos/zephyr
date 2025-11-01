.. zephyr:board:: nsim_arc_v

Overview
********

This platform can be used to run Zephyr RTOS on the widest possible range of Synopsys RISC-V processors in
simulation with `Designware ARC nSIM`_ or run same images on FPGA prototyping platform `HAPS`_. The
platform includes the following features:

* RISC-V processor core, which implements riscv32 ISA
* Virtual serial console (a standard ``ns16550`` UART model)

Supported board targets for that platform are listed below:

* ``nsim_arc_v/rmx100`` - Synopsys RISC-V RMX100 core
* ``nsim_arc_v/rhx100`` - Synopsys RISC-V RHX100 core

.. _board_nsim_arc_v_prop_files:

It is recommended to look at precise description of a particular board target in ``.props``
files in :zephyr_file:`boards/snps/nsim/arc_v/support/` directory to understand
which options are configured and so will be used on invocation of the simulator.

.. warning::
   All nSIM targets are used for demo and testing purposes. They are not meant to
   represent any real system and so might be renamed, removed or modified at any point.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Required Hardware and Software
==============================

To run single-core Zephyr RTOS applications in simulation on this board,
either `DesignWare ARC nSIM`_ or `DesignWare ARC Free nSIM`_ is required.

Building & Running Sample Applications
======================================

Most board targets support building with both GNU and ARC MWDT toolchains, however
there might be exceptions from that, especially for newly added targets. You can check supported
toolchains for the board targets in the corresponding ``.yaml`` file.

I.e. for the ``nsim_arc_v/rmx100`` board we can check :zephyr_file:`boards/snps/nsim/arc_v/nsim_arc_v_rmx100.yaml`
and for the ``nsim_arc_v/rhx100`` board we can check :zephyr_file:`boards/snps/nsim/arc_v/nsim_arc_v_rhx100.yaml`

The supported toolchains are listed in ``toolchain:`` array in ``.yaml`` file, where we can find:

* **zephyr** - implies RISC-V GNU toolchain from Zephyr SDK. You can find more information about
  Zephyr SDK :ref:`here <toolchain_zephyr_sdk>`.
* **cross-compile** - implies RISC-V GNU cross toolchain, which is not a part of Zephyr SDK. Note that
  some (especially new) board targets may declare ``cross-compile`` toolchain support without
  ``zephyr`` toolchain support because corresponding target CPU support hasn't been added to Zephyr
  SDK yet. You can find more information about its usage here: :ref:`here <other_x_compilers>`.
* **arcmwdt** - implies proprietary ARC MWDT toolchain. You can find more information about its
  usage here: :ref:`here <toolchain_designware_arc_mwdt>`.

.. note::
   Note that even if both GNU and MWDT toolchain support is declared for the target some tests or
   samples can be only built with either GNU or MWDT toolchain due to some features limited to a
   particular toolchain.

Use this configuration to run basic Zephyr applications and kernel tests in
nSIM, for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: nsim_arc_v/rmx100
   :goals: flash

This will build an image with the synchronization sample app, boot it using
nSIM, and display the following console output:

.. code-block:: console

      *** Booting Zephyr OS build zephyr-v3.2.0-3948-gd351a024dc87 ***
      thread_a: Hello World from cpu 0 on nsim_arc_v!
      thread_b: Hello World from cpu 0 on nsim_arc_v!
      thread_a: Hello World from cpu 0 on nsim_arc_v!
      thread_b: Hello World from cpu 0 on nsim_arc_v!
      thread_a: Hello World from cpu 0 on nsim_arc_v!

.. note::
   To exit the simulator, use :kbd:`Ctrl+]`, then :kbd:`Ctrl+c`

.. _board_nsim_arc_v_verbose_build:

.. tip::
   You can get more details about the building process by running build in verbose mode. It can be
   done by passing ``-v`` flag to the west: ``west -v build -b nsim_hs samples/synchronization``

Debugging
=========

.. _board_nsim_arc_v_debugging_gdb:

Debugging with GDB
------------------

.. note::
   Debugging on nSIM via GDB is only supported on single-core targets (which use standalone
   nSIM).

.. note::
   The normal ``west debug`` command won't work for debugging applications using nsim boards
   because both the nSIM simulator and the debugger use the same console for
   input / output.
   In case of GDB debugger it's possible to use a separate terminal windows for GDB and nSIM to
   avoid intermixing their output.

After building your application, open two terminal windows. In terminal one, use nSIM to start a GDB
server and wait for a remote connection with following command:

.. code-block:: console

   west debugserver --runner arc-nsim

In terminal two, connect to the GDB server using RISC-V GDB. You can find it in Zephyr SDK:

* you should use :file:`riscv64-zephyr-elf-gdb`

This command loads the symbol table from the elf binary file, for example the
:file:`build/zephyr/zephyr.elf` file:

.. code-block:: console

   riscv64-zephyr-elf-gdb  -ex 'target remote localhost:3333' -ex load build/zephyr/zephyr.elf

Now the debug environment has been set up, and it's possible to debug the application with gdb
commands.

Debugging with lldbac
---------------------

The ``lldbac`` runner is provided as part of the Synopsys ARC MWDT toolchain and supports both
``run-lldbac`` (for integrated debugging) and ``lldbac`` (for client-only mode).

.. note::
   Ensure ``run-lldbac`` and ``lldbac`` (from `ARC MWDT`_) are installed and available in your
   ``PATH``.

The ``lldbac`` runner operates in two modes:

1. **Integrated mode** (``debug``/``flash``): Runner manages the debug server internally
2. **Client-only mode** (``debugserver``): Connect to an externally managed GDB server

Integrated Mode - Simulator (nSIM)
***********************************

For interactive debugging on nSIM simulator, use the ``debug`` command with ``--simulator`` flag:

.. code-block:: console

   west debug --runner lldbac --simulator

The runner uses nSIM properties configured in the board's ``board.cmake`` file. You can override
with ``--nsim-props`` or use a TCF file with ``--tcf``:

.. code-block:: console

   west debug --runner lldbac --simulator --nsim-props=rmx100.props
   west debug --runner lldbac --simulator --tcf=rmx100

.. note::
   The ``lldbac`` runner is designed specifically for interactive debugging workflows. For
   simulator execution without debugging (flash), use the ``arc-nsim`` runner which provides
   efficient non-interactive execution:

   .. code-block:: console

      west flash --runner arc-nsim

   This separation allows ``lldbac`` to focus on debugging features while ``arc-nsim`` handles
   standard execution efficiently.

Integrated Mode - Hardware
***************************

For debugging on physical hardware, provide JTAG configuration via command-line flags:

.. code-block:: console

   west debug --runner lldbac --jtag-device=JtagHs2
   west flash --runner lldbac --jtag-device=JtagHs2

Available hardware options:

* ``--jtag-device``: JTAG device name (required, e.g., JtagHs2)
* ``--jtag``: JTAG adapter type (default: jtag-digilent)
* ``--jtag-frequency``: JTAG clock frequency (default: 500KHz)

GUI Mode
********

To launch debugging with VS Code GUI support, add the ``--gui`` flag:

.. code-block:: console

   west debug --runner lldbac --simulator --gui
   west debug --runner lldbac --jtag-device=JtagHs2 --gui

Client-Only Mode (debugserver)
*******************************

The ``debugserver`` command connects an ``lldbac`` client to an externally managed GDB server. This
enables remote debugging via GDB remote protocol, the primary solution for accessing ARC
probes/targets over network connections.

.. note::
   This is a **non-standard** use of ``debugserver``. Most Zephyr runners use ``debugserver`` to
   *start* a GDB server, but ``lldbac`` uses it to *connect* to an existing server.

First, start a GDB server manually:

For simulator:

.. code-block:: console

   nsimdrv -gdb -port=3333 -propsfile build/zephyr/nsim.props

For hardware (if supported by your setup):

.. code-block:: console

   lldbac gdbserver --jtag :3333

Then connect the debugger client:

.. code-block:: console

   west debugserver --runner lldbac
   west debugserver --runner lldbac --gui
   west debugserver --runner lldbac --gdb-host=remote.server.com --gdb-port=3333

Available client-only options:

* ``--gdb-host``: GDB server hostname/IP (default: localhost)
* ``--gdb-port``: GDB server port (default: 3333)
* ``--gui``: Launch VS Code GUI debugger

.. note::
   Current ``nsim_arc_v`` targets are single-core. Multi-core workflows may require additional
   board-specific integration and are not covered here.

Modifying the configuration
***************************

If modification of existing nsim configuration is required or even there's a need in creation of a
new one it's required to maintain alignment between

* Zephyr OS configuration
* nSIM configuration
* GNU & MWDT toolchain compiler options

.. note::
   The ``.tcf`` configuration files are not supported by Zephyr directly. There are multiple
   reasons for that. ``.tcf`` perfectly suits building of bare-metal single-thread application -
   in that case all the compiler options from ``.tcf`` are passed to the compiler, so all the HW
   features are used by the application and optimal code is being generated.
   The situation is completely different when multi-thread feature-rich operation system is
   considered. Of course it is still possible to build all the code with all the
   options from ``.tcf`` - but that may be far from optimal solution. For example, such approach
   require so save & restore full register context for all tasks (and sometimes even for
   interrupts). And for DSP-enabled or for FPU-enabled systems that leads to dozens of extra
   registers save and restore even if the most of the user and kernel tasks don't actually use
   DSP or FPU. Instead we prefer to fine-tune the HW features usage which (with all its pros)
   require us to maintain them separately from ``.tcf`` configuration.


Zephyr OS configuration
=======================

Zephyr OS configuration is defined via Kconfig and Device tree. These are non RISC-V-specific
mechanisms which are described in :ref:`board porting guide <board_porting_guide>`.

It is advised to look for ``<board_name>_defconfig``, ``<board_name>.dts`` and
``<board_name>.yaml`` as an entry point for board target.

nSIM configuration
==================

nSIM configuration is defined in :ref:`props files <board_nsim_arc_v_prop_files>`.
Generally they are identical to the values from corresponding ``.tcf`` configuration with few
exceptions:

* The UART model is added
* CLINT model is added

GNU & MWDT toolchain compiler options
=====================================

The hardware-specific compiler options are set in corresponding SoC cmake file. For ``nsim_arc_v`` board
it is :zephyr_file:`soc/snps/nsim/arc_v/CMakeLists.txt`.

For the GNU toolchain the basic configuration is set via ``-march`` which is defined in generic code
and based on the selected CPU model via Kconfig. It still can be forcefully set to required value
on SoC level.

.. note::
   The non hardware-specific compiler options like optimizations, library selections, C / C++
   language options are still set in Zephyr generic code. It could be observed by
   :ref:`running build in verbose mode <board_nsim_arc_v_verbose_build>`.

References
**********

.. target-notes::

.. _Designware ARC nSIM: https://www.synopsys.com/dw/ipdir.php?ds=sim_nsim
.. _DesignWare ARC Free nSIM: https://www.synopsys.com/cgi-bin/dwarcnsim/req1.cgi
.. _HAPS: https://www.synopsys.com/verification/prototyping/haps.html
.. _ARC MWDT: https://www.synopsys.com/dw/ipdir.php?ds=sw_metaware
