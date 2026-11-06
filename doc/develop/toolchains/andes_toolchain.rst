.. _toolchain_andes:

Andes Toolchain
###############

#. Download and install a toolchain on your host, either:

   - the toolchain bundled in `AndeSight IDE`_, or
   - a prebuilt Andes toolchain from the `Andes Development Kit releases`_.

   .. note::

      The currently supported release is **ast-v5_4_0-release** and later for
      Linux and MinGW-based Windows toolchain.

#. Choose a toolchain matching your build configuration, see
   :ref:`toolchain_andes_requirements`.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``andes`` or ``andes-clang``.
   - Set :envvar:`ANDES_TOOLCHAIN_PATH` to the toolchain installation directory.

#. For example

   #. Andes GCC:

      .. code-block:: console

         # Linux
         export ZEPHYR_TOOLCHAIN_VARIANT=andes
         export ANDES_TOOLCHAIN_PATH=/path/to/nds32le-elf-mculib-v5

   #. Andes Clang:

      .. code-block:: console

         # Linux
         export ZEPHYR_TOOLCHAIN_VARIANT=andes-clang
         export ANDES_TOOLCHAIN_PATH=/path/to/nds32le-elf-mculib-v5

      Andes Clang toolchain uses LLVM linker by default. You can choose the
      linker via Kconfig:

      - Set ``CONFIG_ANDES_CLANG_USE_LLD=y`` to use the LLVM lld linker.
      - Set ``CONFIG_ANDES_CLANG_USE_LD=y`` to use the GNU ld linker.

.. _toolchain_andes_requirements:

Toolchain Requirements
**********************

Each Andes toolchain provides its runtime libraries (e.g. ``libgcc`` and
``libc``) only for the ABI given by its ``v5*`` suffix, and objects with
different float ABIs cannot be linked together. The Kconfig and the CPU ISA in
devicetree (``riscv,isa-base`` and ``riscv,isa-extensions``) must therefore
match the toolchain, where ``rv32*`` is for ``nds32le`` and ``rv64i`` is for
``nds64le``:

.. list-table::
   :header-rows: 1

   * - Toolchain
     - Required Kconfig
     - ISA base
     - Required ISA extensions
   * - ``v5``
     - :kconfig:option:`CONFIG_FLOAT_HARD` disabled
     - ``rv32i``, ``rv64i``
     - None
   * - ``v5e``
     - :kconfig:option:`CONFIG_FLOAT_HARD` disabled
     - ``rv32e``
     - None
   * - ``v5f``
     - :kconfig:option:`CONFIG_FPU` and :kconfig:option:`CONFIG_FLOAT_HARD`
       enabled
     - ``rv32i``, ``rv64i``
     - ``f`` (without ``d``)
   * - ``v5d``
     - :kconfig:option:`CONFIG_FPU` and :kconfig:option:`CONFIG_FLOAT_HARD`
       enabled
     - ``rv32i``, ``rv64i``
     - ``f``, ``d``

.. _AndeSight IDE: https://www.andestech.com/en/products-solutions/andesight-ide-tools/ide

.. _Andes Development Kit releases: https://github.com/andestech/Andes-Development-Kit/releases
