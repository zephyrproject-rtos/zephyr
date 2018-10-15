.. _custom_cmake_toolchains:

Custom CMake Toolchains
#######################

To use a custom toolchain defined in an external CMake file, export the
following environment variables:

.. code-block:: console

   # Linux and macOS
   export ZEPHYR_TOOLCHAIN_VARIANT=<toolchain name>
   export TOOLCHAIN_ROOT=<path to toolchain>

   # Windows
   set ZEPHYR_TOOLCHAIN_VARIANT=<toolchain name>
   set TOOLCHAIN_ROOT=<path to toolchain>

You can also set them as CMake variables when generating a build
system for a Zephyr application, like so:

.. code-block:: console

   cmake -DZEPHYR_TOOLCHAIN_VARIANT=... -DTOOLCHAIN_ROOT=...

Zephyr will then include the toolchain cmake file located in:
``<path to toolchain>/cmake/toolchain/<toolchain name>.cmake``.
