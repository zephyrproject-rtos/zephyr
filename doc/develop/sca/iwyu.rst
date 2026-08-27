.. _iwyu:

include-what-you-use (IWYU) support
###################################

`include-what-you-use <https://include-what-you-use.org/>`__ (IWYU) is a tool
built on top of Clang and LLVM that analyzes ``#include`` directives in C and
C++ source files. For every translation unit it reports headers that are
included but not used, and symbols that are used but whose defining header is
only included transitively. Acting on its suggestions keeps the set of includes
minimal and explicit.

Installing include-what-you-use
*******************************

``include-what-you-use`` is distributed by most Linux distributions, on ubuntu:

.. code-block:: shell

    sudo apt-get install iwyu

Make sure the ``include-what-you-use`` binary is available in your :envvar:`PATH`.

Run include-what-you-use
************************

.. note::

   IWYU is built on Clang, so building with the LLVM toolchain produces the most
   accurate results.

To run include-what-you-use, :ref:`west build <west-building>` should be called
with a ``-DZEPHYR_SCA_VARIANT=iwyu`` parameter, e.g.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: native_sim
   :gen-args: -DZEPHYR_SCA_VARIANT=iwyu
   :goals: build
   :compact:

The analysis runs alongside the compilation of each source file, and the
suggested include changes are printed to the build output (stderr).

Configuring include-what-you-use
********************************

include-what-you-use can be controlled using specific options. Refer to the
`IWYU documentation <https://github.com/include-what-you-use/include-what-you-use/blob/master/README.md>`__
for the full list of options.

.. list-table::
   :header-rows: 1

   * - Parameter
     - Description
   * - ``IWYU_OPTS``
     - A semicolon separated list of include-what-you-use options. Each option
       is forwarded to the tool with the required ``-Xiwyu`` prefix
       automatically.

These parameters can be passed on the command line, or be set as environment
variables.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: native_sim
   :gen-args: -DZEPHYR_SCA_VARIANT=iwyu -DIWYU_OPTS="--no_comments;--verbose=3"
   :goals: build
   :compact:
