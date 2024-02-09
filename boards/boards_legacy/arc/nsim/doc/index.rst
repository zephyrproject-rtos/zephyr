.. _nsim:

DesignWare ARC nSIM and HAPS FPGA boards
########################################

Overview
********

This platform can be used to run Zephyr RTOS on the widest possible range of ARC processors in
simulation with `Designware ARC nSIM`_ or run same images on FPGA prototyping platform `HAPS`_. The
platform includes the following features:

* ARC processor core, which implements ARCv2 or ARCv3 ISA, please refer to
  :ref:`here <hardware_arch_arc_support_status>` for a complete list of ARC processor families which
  currently supported
* Virtual serial console (a standard ``ns16550`` UART model)

ARC processors are known for being highly customizable and some but not all of the configurations
are currently supported in the Zephyr RTOS for ARC, again please refer to
:ref:`here <hardware_arch_arc_support_status>` for a complete list of supported features.

There are multiple supported sub-configurations for that platform. Some but not all of currently
available configurations are listed below:

* ``nsim_em`` - ARC EM core v4.0 with two register banks, FastIRQ's, MPUv2, DSP options and
  XY-memory
* ``nsim_em_em7d_v22`` - ARC EM core v3.0 with one register bank and FastIRQ's
* ``nsim_em_em11d`` - ARC EM core v4.0 with one register bank, no FastIRQ's, MPUv2, DSP options and
  XY-memory
* ``nsim_sem`` - ARC EM core v4.0 with secure features (thus "SEM", i.e. Secure EM) and MPUv4
* ``nsim_hs`` - ARCv2 HS core v2.1 with two register banks, FastIRQ's and MPUv3
* ``nsim_hs_smp`` - Dual-core ARCv2 HS core v2.1 with two register banks, FastIRQ's and MPUv3
* ``nsim_vpx5`` - ARCv2 VPX5 core, close to vpx5_integer_full template
* ``nsim_hs5x`` - 32-bit ARCv3 HS core with rich set of options
* ``nsim_hs6x`` - 64-bit ARCv3 HS core with rich set of options
* ``nsim_hs5x_smp_12cores`` - SMP 12 cores 32-bit ARCv3 HS platform
* ``nsim_hs6x_smp_12cores`` - SMP 12 cores 64-bit ARCv3 HS platform

.. _board_arc_nsim_prop_args_files:

It is recommended to look at precise description of a particular sub-configuration in either
``.props`` or ``.args`` files in :zephyr_file:`boards/arc/nsim/support/` directory to understand
which options are configured and so will be used on invocation of the simulator.

In case of single-core configurations it would be ``.props`` file which contains configuration
for nSIM simulator and ``.args`` file which contains configuration for MetaWare debugger (MDB).
Note that these files contain identical HW configuration and meant to be used with the corresponding
tool: ``.props`` file for nSIM simulator and ``.args`` file for MDB (which internally uses nSIM for
simulation anyway).

.. hint::
   If different behavior is observed during execution or debugging of a particular application
   (especially after creation of a new board or modification of the existing one) make sure features
   defined in ``.props`` and ``.args`` are semantically identical (unfortunately options of
   nSIM & MDB don't exactly match, so care should be taken).

I.e. for the single-core ``nsim_hs5x`` platform there are
:zephyr_file:`boards/arc/nsim/support/nsim_hs5x.props` and
:zephyr_file:`boards/arc/nsim/support/mdb_hs5x.args`.

For the multi-core configurations there is only ``.args`` file as the multi-core configuration
can only be instantiated with help of MDB.

I.e. for the multi-core ``nsim_hs5x_smp`` platform there is only
:zephyr_file:`boards/arc/nsim/support/mdb_hs5x_smp.args`.

.. warning::
   All nSIM/MDB configurations are used for demo and testing purposes. They are not meant to
   represent any real system and so might be renamed, removed or modified at any point.

Programming and Debugging
*************************

Required Hardware and Software
==============================

To run single-core Zephyr RTOS applications in simulation on this board,
either `DesignWare ARC nSIM`_ or `DesignWare ARC Free nSIM`_ is required.

To run multi-core Zephyr RTOS applications in simulation on this board,
`DesignWare ARC nSIM`_ and MetaWare Debugger from `ARC MWDT`_ are required.

To run Zephyr RTOS applications on FPGA-based `HAPS`_ platform,
MetaWare Debugger from `ARC MWDT`_ is required as well as the HAPS platform itself.

Building & Running Sample Applications
======================================

Most board sub-configurations support building with both GNU and ARC MWDT toolchains, however
there might be exceptions from that, especially for newly added targets. You can check supported
toolchains for the sub-configurations in the corresponding ``.yaml`` file.

I.e. for the ``nsim_hs5x`` board we can check :zephyr_file:`boards/arc/nsim/nsim_hs5x.yaml`

The supported toolchains are listed in ``toolchain:`` array in ``.yaml`` file, where we can find:

* **zephyr** - implies ARC GNU toolchain from Zephyr SDK. You can find more information about
  Zephyr SDK :ref:`here <toolchain_zephyr_sdk>`.
* **cross-compile** - implies ARC GNU cross toolchain, which is not a part of Zephyr SDK. Note that
  some (especially new) sub-configurations may declare ``cross-compile`` toolchain support without
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
   :board: nsim_em
   :goals: flash

This will build an image with the synchronization sample app, boot it using
nSIM, and display the following console output:

.. code-block:: console

      *** Booting Zephyr OS build zephyr-v3.2.0-3948-gd351a024dc87 ***
      thread_a: Hello World from cpu 0 on nsim!
      thread_b: Hello World from cpu 0 on nsim!
      thread_a: Hello World from cpu 0 on nsim!
      thread_b: Hello World from cpu 0 on nsim!
      thread_a: Hello World from cpu 0 on nsim!


.. note::
   To exit the simulator, use :kbd:`Ctrl+]`, then :kbd:`Ctrl+c`

.. _board_arc_nsim_verbose_build:

.. tip::
   You can get more details about the building process by running build in verbose mode. It can be
   done by passing ``-v`` flag to the west: ``west -v build -b nsim_hs samples/synchronization``

You can run applications built for ``nsim`` board not only on nSIM simulation itself, but also on
FPGA based HW platform `HAPS`_. To run previously built application on HAPS do:

.. code-block:: console

   west flash --runner mdb-hw

.. note::
   To run on HAPS, in addition to proper build and flash Zephyr image, you need setup HAPS itself
   as well as flash proper built FPGA image (aka .bit-file). This instruction doesn't cover those
   steps, so you need to follow HAPS manual.

Debugging
=========

.. _board_arc_nsim_debugging_mwdt:

Debugging with MDB
------------------

.. note::
   We strongly recommend to debug with MetaWare debugger (MDB) because it:

   * Supports wider range of ARC hardware features
   * Allows to debug both single-core and multi-core ``nsim`` targets.
   * Allows to debug on `HAPS`_ platform.

You can use the following command to start GUI debugging when running application on nSIM simulator
(regardless if single- or multi-core configuration is used):

.. code-block:: console

   west debug --runner mdb-nsim

You can use the following command to start GUI debugging when running application on `HAPS`_
platform:

.. code-block:: console

   west debug --runner mdb-hw

.. tip::
   The ``west debug`` (as well as ``west flash``) is just a wrapper script and so it's possible to
   extract the exact commands which are called in it by running it in verbose mode. For that you
   need to pass ``-v`` flag to the wrapper. For example, if you run the following command:

   .. code-block:: console

      west -v debug --runner mdb-nsim

   it will produce the following output (the ``nsim_hs5x_smp`` configuration was used for that
   example):

   .. code-block:: console

       < *snip* >
      -- west debug: using runner mdb-nsim
      runners.mdb-nsim: mdb -pset=1 -psetname=core0 -nooptions -nogoifmain -toggle=include_local_symbols=1 -nsim @/path/zephyr/boards/arc/nsim/support/mdb_hs5x_smp.args /path/zephyr/build/zephyr/zephyr.elf
      runners.mdb-nsim: mdb -pset=2 -psetname=core1 -prop=download=2 -nooptions -nogoifmain -toggle=include_local_symbols=1 -nsim @/path/zephyr/boards/arc/nsim/support/mdb_hs5x_smp.args /path/zephyr/build/zephyr/zephyr.elf
      runners.mdb-nsim: mdb -multifiles=core1,core0 -OKN

   From that output it's possible to extract MDB commands used for setting-up the GUI debugging
   session:

   .. code-block:: console

      mdb -pset=1 -psetname=core0 -nooptions -nogoifmain -toggle=include_local_symbols=1 -nsim @/path/zephyr/boards/arc/nsim/support/mdb_hs5x_smp.args /path/zephyr/build/zephyr/zephyr.elf
      mdb -pset=2 -psetname=core1 -prop=download=2 -nooptions -nogoifmain -toggle=include_local_symbols=1 -nsim @/path/zephyr/boards/arc/nsim/support/mdb_hs5x_smp.args /path/zephyr/build/zephyr/zephyr.elf
      mdb -multifiles=core1,core0 -OKN

   Then it's possible to use them directly or in some machinery if required.

   .. warning::
      It is strongly recommended to not rely on the mdb command line options listed above but
      extract it yourself for your configuration.

   .. note::
      In case of execution or debugging with MDB on multi-core configuration on nSIM
      simulator without ``west flash`` and ``west debug`` wrappers it's necessary to
      set :envvar:`NSIM_MULTICORE` environment variable to ``1``. If you are using ``west flash`` or
      ``west debug`` it's done automatically by wrappers.

      Without :envvar:`NSIM_MULTICORE` environment variable set to 1, MDB will simulate 2 separate
      ARC cores which don't share any memory regions with each other and so SMP-enabled code won't
      work as expected.

Debugging with GDB
------------------

.. note::
   Debugging on nSIM via GDB is only supported on single-core configurations (which use standalone
   nSIM). However if it's possible to launch application on multi-core nsim target that means you
   can simply :ref:`debug with MDB debugger <board_arc_nsim_debugging_mwdt>`.
   It's the nSIM with ARC GDB restriction, real HW multi-core ARC targets can be debugged with ARC
   GDB.

.. note::
   Currently debugging with GDB is not supported on `HAPS`_ platform.

.. note::
   The normal ``west debug`` command won't work for debugging applications using nsim boards
   because both the nSIM simulator and the debugger (either GDB or MDB) use the same console for
   input / output.
   In case of GDB debugger it's possible to use a separate terminal windows for GDB and nSIM to
   avoid intermixing their output. For the MDB debugger simply use GUI mode.

After building your application, open two terminal windows. In terminal one, use nSIM to start a GDB
server and wait for a remote connection with following command:

.. code-block:: console

   west debugserver --runner arc-nsim

In terminal two, connect to the GDB server using ARC GDB. You can find it in Zephyr SDK:

* for the ARCv2 targets you should use :file:`arc-zephyr-elf-gdb`
* for the ARCv3 targets you should use :file:`arc64-zephyr-elf-gdb`

This command loads the symbol table from the elf binary file, for example the
:file:`build/zephyr/zephyr.elf` file:

.. code-block:: console

   arc-zephyr-elf-gdb  -ex 'target remote localhost:3333' -ex load build/zephyr/zephyr.elf

Now the debug environment has been set up, and it's possible to debug the application with gdb
commands.

Modifying the configuration
***************************

If modification of existing nsim configuration is required or even there's a need in creation of a
new one it's required to maintain alignment between

* Zephyr OS configuration
* nSIM & MDB configuration
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

Zephyr OS configuration is defined via Kconfig and Device tree. These are non ARC-specific
mechanisms which are described in :ref:`board porting guide <board_porting_guide>`.

It is advised to look for ``<board_name>_defconfig``, ``<board_name>.dts`` and
``<board_name>.yaml`` as an entry point for board configuration.

nSIM configuration
==================

nSIM configuration is defined in :ref:`props and args files <board_arc_nsim_prop_args_files>`.
Generally they are identical to the values from corresponding ``.tcf`` configuration with few
exceptions:

* The UART model is added (to both ``.props`` and ``.args`` files).
* Options to fine-tuned MDB behavior are added (to ``.args`` files only) to disable MDB profiling
  and fine-tune MDB behavior on multi-core systems.

GNU & MWDT toolchain compiler options
=====================================

The hardware-specific compiler options are set in corresponding SoC cmake file. For ``nsim`` board
it is :zephyr_file:`soc/arc/snps_nsim/CMakeLists.txt`.

For the GNU toolchain the basic configuration is set via ``-mcpu`` which is defined in generic code
and based on the selected CPU model via Kconfig. It still can be forcefully set to required value
on SoC level.

For the MWDT toolchain all hardware-specific compiler options are set directly in SoC
``CMakeLists.txt``.

.. note::
   The non hardware-specific compiler options like optimizations, library selections, C / C++
   language options are still set in Zephyr generic code. It could be observed by
   :ref:`running build in verbose mode <board_arc_nsim_verbose_build>`.

References
**********

.. _Designware ARC nSIM: https://www.synopsys.com/dw/ipdir.php?ds=sim_nsim
.. _DesignWare ARC Free nSIM: https://www.synopsys.com/cgi-bin/dwarcnsim/req1.cgi
.. _HAPS: https://www.synopsys.com/verification/prototyping/haps.html
.. _ARC MWDT: https://www.synopsys.com/dw/ipdir.php?ds=sw_metaware
