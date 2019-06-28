.. _custom_cmake_toolchains:

Custom CMake Toolchains
#######################

To use a custom toolchain defined in an external CMake file, :ref:`set these
environment variables <env_vars>`:

- Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to your toolchain's name
- Set :envvar:`TOOLCHAIN_ROOT` to the path to the directory containing your
  toolchain's CMake configuration files.

Zephyr will then include the toolchain cmake files located in the
:file:`TOOLCHAIN_ROOT` directory:

- :file:`cmake/toolchain/generic.cmake`: configures the toolchain for "generic"
  use, which mostly means running the C preprocessor on the generated
  :ref:`device-tree` file.
- :file:`cmake/toolchain/target.cmake`: configures the toolchain for "target"
  use, i.e. building Zephyr and your application's source code.

See the zephyr files :zephyr_file:`cmake/generic_toolchain.cmake` and
:zephyr_file:`cmake/target_toolchain.cmake` for more details on what your
:file:`generic.cmake` and :file:`target.cmake` files should contain.

You can also set ``ZEPHYR_TOOLCHAIN_VARIANT`` and ``TOOLCHAIN_ROOT`` as CMake
variables when generating a build system for a Zephyr application, like so:

.. code-block:: console

   west build ... -- -DZEPHYR_TOOLCHAIN_VARIANT=... -DTOOLCHAIN_ROOT=...

.. code-block:: console

   cmake -DZEPHYR_TOOLCHAIN_VARIANT=... -DTOOLCHAIN_ROOT=...

If you do this, ``-C <initial-cache>`` `cmake option`_ may useful. If you save
your :makevar:`ZEPHYR_TOOLCHAIN_VARIANT`, :makevar:`TOOLCHAIN_ROOT`, and other
settings in a file named :file:`my-toolchain.cmake`, you can then invoke cmake
as ``cmake -C my-toolchain.cmake ...`` to save typing.

.. _cmake option:
   https://cmake.org/cmake/help/latest/manual/cmake.1.html#options
