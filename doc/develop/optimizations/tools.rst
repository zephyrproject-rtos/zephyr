.. _optimization_tools:

Optimization Tools
##################

The available optimization tools let you analyse :ref:`footprint_tools`
and :ref:`data_structures` using different build system targets.

.. _footprint_tools:

Footprint and Memory Usage
**************************

The build system offers 3 targets to view and analyse RAM, ROM and stack usage
in generated images. The tools run on the final image and give information
about size of symbols and code being used in both RAM and ROM. Additionally,
with features available through the compiler, we can also generate worst-case
stack usage analysis.

Some of the tools mentioned in this section are organizing their output based
on the physical organization of the symbols. As some symbols might be external
to the project's tree structure, or might lack metadata needed to display them
by name, the following top-level containers are used to group such symbols:

* Hidden - The RAM and ROM reports list all processing symbols with no matching
  mapped files in the Hidden category.

  This means that the file for the listed symbol was not added to the metadata file,
  was empty, or was undefined. The tool was unable to get the name
  of the function for the given symbol nor identify where it comes from.

* No paths - The RAM and ROM reports list all processing symbols with relative paths
  in the No paths category.

  This means that the listed symbols cannot be placed in the tree structure
  of the report at an absolute path under one specific file. The tool was able
  to get the name of the function, but it was unable to identify where it comes from.

  .. note::

     You can have multiple cases of the same function, and the No paths category
     will list the sum of these in one entry.


Build Target: ram_report
========================

List all compiled objects and their RAM usage in a tabular form with bytes
per symbol and the percentage it uses. The data is grouped based on the file
system location of the object in the tree and the file containing the symbol.

Use the ``ram_report`` target with your board, as in the following example.

.. zephyr-app-commands::
    :tool: all
    :app: samples/hello_world
    :board: reel_board
    :goals: ram_report

These commands will generate something similar to the output below::

    Path                                                                       Size    %
    ========================================================================================
    Root                                                                       4637 100.00%
    ├── (hidden)                                                                  4   0.09%
    ├── (no paths)                                                             2748  59.26%
    │   ├── _cpus_active                                                          4   0.09%
    │   ├── _kernel                                                              32   0.69%
    │   ├── _sw_isr_table                                                       384   8.28%
    │   ├── cli.1                                                                16   0.35%
    │   ├── on.2                                                                  4   0.09%
    │   ├── poll_out_lock.0                                                       4   0.09%
    │   ├── z_idle_threads                                                      128   2.76%
    │   ├── z_interrupt_stacks                                                 2048  44.17%
    │   └── z_main_thread                                                       128   2.76%
    ├── WORKSPACE                                                               184   3.97%
    │   └── modules                                                             184   3.97%
    │       └── hal                                                             184   3.97%
    │           └── nordic                                                      184   3.97%
    │               └── nrfx                                                    184   3.97%
    │                   └── drivers                                             184   3.97%
    │                       └── src                                             184   3.97%
    │                           ├── nrfx_clock.c                                  8   0.17%
    │                           │   └── m_clock_cb                                8   0.17%
    │                           ├── nrfx_gpiote.c                               132   2.85%
    │                           │   └── m_cb                                    132   2.85%
    │                           ├── nrfx_ppi.c                                    4   0.09%
    │                           │   └── m_channels_allocated                      4   0.09%
    │                           └── nrfx_twim.c                                  40   0.86%
    │                               └── m_cb                                     40   0.86%
    └── ZEPHYR_BASE                                                            1701  36.68%
        ├── arch                                                                  5   0.11%
        │   └── arm                                                               5   0.11%
        │       └── core                                                          5   0.11%
        │           ├── mpu                                                       1   0.02%
        │           │   └── arm_mpu.c                                             1   0.02%
        │           │       └── static_regions_num                                1   0.02%
        │           └── tls.c                                                     4   0.09%
        │               └── z_arm_tls_ptr                                         4   0.09%
        ├── drivers                                                             258   5.56%
        │   ├── ...                                                             ...    ...%
    ========================================================================================
                                                                               4637


Build Target: rom_report
========================

List all compiled objects and their ROM usage in a tabular form with bytes
per symbol and the percentage it uses. The data is grouped based on the file
system location of the object in the tree and the file containing the symbol.

Use the ``rom_report`` target with your board, as in the following example.

.. zephyr-app-commands::
    :tool: all
    :app: samples/hello_world
    :board: reel_board
    :goals: rom_report

These commands will generate something similar to the output below::

    Path                                                                       Size    %
    ========================================================================================
    Root                                                                      21652 100.00%
    ├── ...                                                                     ...    ...%
    └── ZEPHYR_BASE                                                           13378  61.79%
        ├── arch                                                               1718   7.93%
        │   └── arm                                                            1718   7.93%
        │       └── core                                                       1718   7.93%
        │           ├── cortex_m                                               1020   4.71%
        │           │   ├── fault.c                                             620   2.86%
        │           │   │   ├── bus_fault.constprop.0                           108   0.50%
        │           │   │   ├── mem_manage_fault.constprop.0                    120   0.55%
        │           │   │   ├── usage_fault.constprop.0                          84   0.39%
        │           │   │   ├── z_arm_fault                                     292   1.35%
        │           │   │   └── z_arm_fault_init                                 16   0.07%
        │           │   ├── ...                                                 ...    ...%
        ├── boards                                                               32   0.15%
        │   └── arm                                                              32   0.15%
        │       └── reel_board                                                   32   0.15%
        │           └── board.c                                                  32   0.15%
        │               ├── __init_board_reel_board_init                          8   0.04%
        │               └── board_reel_board_init                                24   0.11%
        ├── build                                                               194   0.90%
        │   └── zephyr                                                          194   0.90%
        │       ├── isr_tables.c                                                192   0.89%
        │       │   └── _irq_vector_table                                       192   0.89%
        │       └── misc                                                          2   0.01%
        │           └── generated                                                 2   0.01%
        │               └── configs.c                                             2   0.01%
        │                   └── _ConfigAbsSyms                                    2   0.01%
        ├── drivers                                                            6162  28.46%
        │   ├── ...                                                             ...    ...%
    ========================================================================================
                                                                                    21652

Build Target: puncover
======================

This target uses a third-party tool called puncover which can be found at
https://github.com/HBehrens/puncover. When this target is built, it will
launch a local web server which will allow you to open a web client and browse
the files and view their ROM, RAM, and stack usage.

Before you can use this
target, install the puncover Python module::

    pip3 install git+https://github.com/HBehrens/puncover --user

.. warning::

   This is a third-party tool that might or might not be working at any given
   time. Please check the GitHub issues, and report new problems to the
   project maintainer.

After you installed the Python module, use ``puncover`` target with your board,
as in the following example.

.. zephyr-app-commands::
    :tool: all
    :app: samples/hello_world
    :board: reel_board
    :goals: puncover


To view worst-case stack usage analysis, build this with the
:kconfig:option:`CONFIG_STACK_USAGE` enabled.

.. zephyr-app-commands::
    :tool: all
    :app: samples/hello_world
    :board: reel_board
    :goals: puncover
    :gen-args: -DCONFIG_STACK_USAGE=y


.. _data_structures:

Data Structures
****************


Build Target: pahole
=====================

Poke-a-hole (pahole) is an object-file analysis tool to find the size of
the data structures, and the holes caused due to aligning the data
elements to the word-size of the CPU by the compiler.

Poke-a-hole (pahole) must be installed prior to using this target. It can be
obtained from https://git.kernel.org/pub/scm/devel/pahole/pahole.git and is
available in the dwarves package in both fedora and ubuntu::

    sudo apt-get install dwarves

Alternatively, you can get it from fedora::

    sudo dnf install dwarves

After you installed the package, use ``pahole`` target with your board,
as in the following example.

.. zephyr-app-commands::
    :tool: all
    :app: samples/hello_world
    :board: reel_board
    :goals: pahole

Pahole will generate something similar to the output below in the console::

    /* Used at: zephyr/isr_tables.c */
    /* <80> ../include/sw_isr_table.h:30 */
    struct _isr_table_entry {
            void *                     arg;                  /*     0     4 */
            void                       (*isr)(void *);       /*     4     4 */

            /* size: 8, cachelines: 1, members: 2 */
            /* last cacheline: 8 bytes */
    };
    /* Used at: zephyr/isr_tables.c */
    /* <eb> ../include/arch/arm/aarch32/cortex_m/mpu/arm_mpu_v7m.h:134 */
    struct arm_mpu_region_attr {
            uint32_t                   rasr;                 /*     0     4 */

            /* size: 4, cachelines: 1, members: 1 */
            /* last cacheline: 4 bytes */
    };
    /* Used at: zephyr/isr_tables.c */
    /* <112> ../include/arch/arm/aarch32/cortex_m/mpu/arm_mpu.h:24 */
    struct arm_mpu_region {
            uint32_t                   base;                 /*     0     4 */
            const char  *              name;                 /*     4     4 */
            arm_mpu_region_attr_t      attr;                 /*     8     4 */

            /* size: 12, cachelines: 1, members: 3 */
            /* last cacheline: 12 bytes */
    };
    ...
    ...
