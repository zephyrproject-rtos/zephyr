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

- :file:`cmake/toolchain/<toolchain name>/generic.cmake`: configures the
  toolchain for "generic" use, which mostly means running the C preprocessor
  on the generated
  :ref:`devicetree` file.
- :file:`cmake/toolchain/<toolchain name>/target.cmake`: configures the
  toolchain for "target" use, i.e. building Zephyr and your application's
  source code.

Here <toolchain name> is the same as the name provided in
:envvar:`ZEPHYR_TOOLCHAIN_VARIANT`
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

Zephyr includes :file:`include/toolchain.h` which again includes a toolchain
specific header based on the compiler identifier, such as ``__llvm__`` or
``__GNUC__``.
Some custom compilers identify themselves as the compiler on which they are
based, for example ``llvm`` which then gets the :file:`toolchain/llvm.h` included.
This included file may though not be right for the custom toolchain. In order
to solve this, and thus to get the :file:`include/other.h` included instead,
add the set(TOOLCHAIN_USE_CUSTOM 1) cmake line to the generic.cmake and/or
target.cmake files located under
:file:`<TOOLCHAIN_ROOT>/cmake/toolchain/<toolchain name>/`.

When :makevar:`TOOLCHAIN_USE_CUSTOM` is set, the :file:`other.h` must be
available out-of-tree and it must include the correct header for the custom
toolchain.
A good location for the :file:`other.h` header file, would be a
directory under the directory specified in :envvar:`TOOLCHAIN_ROOT` as
:file:`include/toolchain`.
To get the toolchain header included in zephyr's build, the
:makevar:`USERINCLUDE` can be set to point to the include directory, as shown
here:

.. code-block:: console

   west build -- -DZEPHYR_TOOLCHAIN_VARIANT=... -DTOOLCHAIN_ROOT=... -DUSERINCLUDE=...

.. _cmake option:
   https://cmake.org/cmake/help/latest/manual/cmake.1.html#options
