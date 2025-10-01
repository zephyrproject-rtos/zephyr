.. _toolchain_andes:

Andes Toolchain
###############

#. Download and extract a toolchain from `Andes-Development-Kit`_ for your
   operating system.

   .. note::

      The currently supported release is **ast-v5_3_3-release** and later for
      Linux and MinGW-based Windows toolchain. Cygwin-based Windows toolchains
      (e.g., *ast-v5_3_3-release-windows*) are **not** supported.

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

      - Set :envvar:`CONFIG_LLVM_USE_LLD=y` to use LLVM linker.
      - set :envvar:`CONFIG_LLVM_USE_LD=y` to use the GNU LD linker.

.. _Andes-Development-Kit: https://github.com/andestech/Andes-Development-Kit/releases
