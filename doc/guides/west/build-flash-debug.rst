.. _west-build-flash-debug:

Building, Flashing and Debugging
################################

Zephyr provides several :ref:`west extension commands <west-extensions>` for
building, flashing, and interacting with Zephyr programs running on a board:
``build``, ``flash``, ``debug``, ``debugserver`` and ``attach``.

For information on adding board support for the flashing and debugging
commands, see :ref:`flash-and-debug-support` in the board porting guide.

.. Add a per-page contents at the top of the page. This page is nested
   deeply enough that it doesn't have any subheadings in the main nav.

.. only:: html

   .. contents::
      :local:

.. _west-building:

Building: ``west build``
************************

.. tip:: Run ``west build -h`` for a quick overview.

The ``build`` command helps you build Zephyr applications from source. You can
use :ref:`west config <west-config-cmd>` to configure its behavior.

Its default behavior tries to "do what you mean":

- If there is a Zephyr build directory named :file:`build` in your current
  working directory, it is incrementally re-compiled. The same is true if you
  run ``west build`` from a Zephyr build directory.

- Otherwise, if you run ``west build`` from a Zephyr application's source
  directory and no build directory is found, a new one is created and the
  application is compiled in it.

Basics
======

The easiest way to use ``west build`` is to go to an application's root
directory (i.e. the folder containing the application's :file:`CMakeLists.txt`)
and then run::

  west build -b <BOARD>

Where ``<BOARD>`` is the name of the board you want to build for. This is
exactly the same name you would supply to CMake if you were to invoke it with:
``cmake -DBOARD=<BOARD>``.

.. tip::

   You can use the :ref:`west boards <west-boards>` command to list all
   supported boards.

A build directory named :file:`build` will be created, and the application will
be compiled there after ``west build`` runs CMake to create a build system in
that directory. If ``west build`` finds an existing build directory, the
application is incrementally re-compiled there without re-running CMake. You
can force CMake to run again with ``--cmake``.

You don't need to use the ``--board`` option if you've already got an existing
build directory; ``west build`` can figure out the board from the CMake cache.
For new builds, the ``--board`` option, :envvar:`BOARD` environment variable,
or ``build.board`` configuration option are checked (in that order).

Examples
========

Here are some ``west build`` usage examples, grouped by area.

Forcing CMake to Run Again
--------------------------

To force a CMake re-run, use the ``--cmake`` (or ``--c``) option::

  west build -c

Setting a Default Board
-----------------------

To configure ``west build`` to build for the ``reel_board`` by default::

  west config build.board reel_board

(You can use any other board supported by Zephyr here; it doesn't have to be
``reel_board``.)

.. _west-building-dirs:

Setting Source and Build Directories
------------------------------------

To set the application source directory explicitly, give its path as a
positional argument::

  west build -b <BOARD> path/to/source/directory

To set the build directory explicitly, use ``--build-dir`` (or ``-d``)::

  west build -b <BOARD> --build-dir path/to/build/directory

To change the default build directory from :file:`build`, use the
``build.dir-fmt`` configuration option. This lets you name build
directories using format strings, like this::

  west config build.dir-fmt "build/{board}/{app}"

With the above, running ``west build -b reel_board samples/hello_world`` will
use build directory :file:`build/reel_board/hello_world`.  See
:ref:`west-building-config` for more details on this option.

Setting the Build System Target
-------------------------------

To specify the build system target to run, use ``--target`` (or ``-t``).

For example, on host platforms with QEMU, you can use the ``run`` target to
build and run the :ref:`hello_world` sample for the emulated :ref:`qemu_x86
<qemu_x86>` board in one command::

  west build -b qemu_x86 -t run samples/hello_world

As another example, to use ``-t`` to list all build system targets::

  west build -t help

As a final example, to use ``-t`` to run the ``pristine`` target, which deletes
all the files in the build directory::

  west build -t pristine

.. _west-building-pristine:

Pristine Builds
---------------

A *pristine* build directory is essentially a new build directory. All
byproducts from previous builds have been removed.

To force ``west build`` make the build directory pristine before re-running
CMake to generate a build system, use the ``--pristine=always`` (or
``-p=always``) option.

Giving ``--pristine`` or ``-p`` without a value has the same effect as giving
it the value ``always``. For example, these commands are equivalent::

  west build -p -b reel_board samples/hello_world
  west build -p=always -b reel_board samples/hello_world

By default, ``west build`` applies a heuristic to detect if the build directory
needs to be made pristine. This is the same as using ``--pristine=auto``.

.. tip::

   You can run ``west config build.pristine always`` to always do a pristine
   build, or ``west config build.pristine never`` to disable the heuristic.
   See the ``west build`` :ref:`west-building-config` for details.

.. _west-building-verbose:

Verbose Builds
--------------

To print the CMake and compiler commands run by ``west build``, use the global
west verbosity option, ``-v``::

  west -v build -b reel_board samples/hello_world

.. _west-building-generator:
.. _west-building-cmake-args:

One-Time CMake Arguments
------------------------

To pass additional arguments to the CMake invocation performed by ``west
build``, pass them after a ``--`` at the end of the command line.

.. important::

   Passing additional CMake arguments like this forces ``west build`` to re-run
   CMake, even if a build system has already been generated.

   After using ``--`` once to generate the build directory, use ``west build -d
   <build-dir>`` on subsequent runs to do incremental builds.

For example, to use the Unix Makefiles CMake generator instead of Ninja (which
``west build`` uses by default), run::

  west build -b reel_board -- -G'Unix Makefiles'

To use Unix Makefiles and set `CMAKE_VERBOSE_MAKEFILE`_ to ``ON``::

  west build -b reel_board -- -G'Unix Makefiles' -DCMAKE_VERBOSE_MAKEFILE=ON

Notice how the ``--`` only appears once, even though multiple CMake arguments
are given. All command-line arguments to ``west build`` after a ``--`` are
passed to CMake.

.. _west-building-dtc-overlay-file:

To set :ref:`DTC_OVERLAY_FILE <important-build-vars>` to
:file:`enable-modem.overlay`, using that file as a
:ref:`devicetree overlay <dt-guide>`::

  west build -b reel_board -- -DDTC_OVERLAY_FILE=enable-modem.overlay

To merge the :file:`file.conf` Kconfig fragment into your build's
:file:`.config`::

  west build -- -DOVERLAY_CONFIG=file.conf

.. _west-building-cmake-config:

Permanent CMake Arguments
-------------------------

The previous section describes how to add CMake arguments for a single ``west
build`` command. If you want to save CMake arguments for ``west build`` to use
every time it generates a new build system instead, you should use the
``build.cmake-args`` configuration option. Whenever ``west build`` runs CMake
to generate a build system, it splits this option's value according to shell
rules and includes the results in the ``cmake`` command line.

Remember that, by default, ``west build`` **tries to avoid generating a new
build system if one is present** in your build directory. Therefore, you need
to either delete any existing build directories or do a :ref:`pristine build
<west-building-pristine>` after setting ``build.cmake-args`` to make sure it
will take effect.

For example, to always enable :makevar:`CMAKE_EXPORT_COMPILE_COMMANDS`, you can
run::

  west config build.cmake-args -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

(The extra ``--`` is used to force the rest of the command to be treated as a
positional argument. Without it, :ref:`west config <west-config-cmd>` would
treat the ``-DVAR=VAL`` syntax as a use of its ``-D`` option.)

To enable :makevar:`CMAKE_VERBOSE_MAKEFILE`, so CMake always produces a verbose
build system::

  west config build.cmake-args -- -DCMAKE_VERBOSE_MAKEFILE=ON

To save more than one argument in ``build.cmake-args``, use a single string
whose value can be split into distinct arguments (``west build`` uses the
Python function `shlex.split()`_ internally to split the value).

.. _shlex.split(): https://docs.python.org/3/library/shlex.html#shlex.split

For example, to enable both :makevar:`CMAKE_EXPORT_COMPILE_COMMANDS` and
:makevar:`CMAKE_VERBOSE_MAKEFILE`::

  west config build.cmake-args -- "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_VERBOSE_MAKEFILE=ON"

If you want to save your CMake arguments in a separate file instead, you can
combine CMake's ``-C <initial-cache>`` option with ``build.cmake-args``. For
instance, another way to set the options used in the previous example is to
create a file named :file:`~/my-cache.cmake` with the following contents:

.. code-block:: cmake

   set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "")
   set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "")

Then run::

  west config build.cmake-args "-C ~/my-cache.cmake"

See the `cmake(1) manual page`_ and the `set() command`_ documentation for
more details.

.. _cmake(1) manual page:
   https://cmake.org/cmake/help/latest/manual/cmake.1.html

.. _set() command:
   https://cmake.org/cmake/help/latest/command/set.html

.. _west-building-config:

Configuration Options
=====================

You can :ref:`configure <west-config-cmd>` ``west build`` using these options.

.. NOTE: docs authors: keep this table sorted alphabetically

.. list-table::
   :widths: 10 30
   :header-rows: 1

   * - Option
     - Description
   * - ``build.board``
     - String. If given, this the board used by :ref:`west build
       <west-building>` when ``--board`` is not given and :envvar:`BOARD`
       is unset in the environment.
   * - ``build.board_warn``
     - Boolean, default ``true``. If ``false``, disables warnings when
       ``west build`` can't figure out the target board.
   * - ``build.cmake-args``
     - String. If present, the value will be split according to shell rules and
       passed to CMake whenever a new build system is generated. See
       :ref:`west-building-cmake-config`.
   * - ``build.dir-fmt``
     - String, default ``build``. The build folder format string, used by
       west whenever it needs to create or locate a build folder. The currently
       available arguments are:

         - ``board``: The board name
         - ``source_dir``: The relative path from the current working directory
           to the source directory. If the current working directory is inside
           the source directory this will be set to an empty string.
         - ``app``: The name of the source directory.
   * - ``build.generator``
     - String, default ``Ninja``. The `CMake Generator`_ to use to create a
       build system. (To set a generator for a single build, see the
       :ref:`above example <west-building-generator>`)
   * - ``build.guess-dir``
     - String, instructs west whether to try to guess what build folder to use
       when ``build.dir-fmt`` is in use and not enough information is available
       to resolve the build folder name. Can take these values:

         - ``never`` (default): Never try to guess, bail out instead and
           require the user to provide a build folder with ``-d``.
         - ``runners``: Try to guess the folder when using any of the 'runner'
           commands.  These are typically all commands that invoke an external
           tool, such as ``flash`` and ``debug``.
   * - ``build.pristine``
     - String. Controls the way in which ``west build`` may clean the build
       folder before building. Can take the following values:

         - ``never`` (default): Never automatically make the build folder
           pristine.
         - ``auto``:  ``west build`` will automatically make the build folder
           pristine before building, if a build system is present and the build
           would fail otherwise (e.g. the user has specified a different board
           or application from the one previously used to make the build
           directory).
         - ``always``: Always make the build folder pristine before building, if
           a build system is present.

.. _west-flashing:

Flashing: ``west flash``
************************

.. tip:: Run ``west flash -h`` for additional help.

Basics
======

From a Zephyr build directory, re-build the binary and flash it to
your board::

  west flash

Without options, the behavior is the same as ``ninja flash`` (or
``make flash``, etc.).

To specify the build directory, use ``--build-dir`` (or ``-d``)::

  west flash --build-dir path/to/build/directory

If you don't specify the build directory, ``west flash`` searches for one in
:file:`build`, then the current working directory. If you set the
``build.dir-fmt`` configuration option (see :ref:`west-building-dirs`), ``west
flash`` searches there instead of :file:`build`.

Choosing a Runner
=================

If your board's Zephyr integration supports flashing with multiple
programs, you can specify which one to use using the ``--runner`` (or
``-r``) option. For example, if West flashes your board with
``nrfjprog`` by default, but it also supports JLink, you can override
the default with::

  west flash --runner jlink

You can override the default flash runner at build time by using the
``BOARD_FLASH_RUNNER`` CMake variable, and the debug runner with
``BOARD_DEBUG_RUNNER``.

For example::

  # Set the default runner to "jlink", overriding the board's
  # usual default.
  west build [...] -- -DBOARD_FLASH_RUNNER=jlink

See :ref:`west-building-cmake-args` and :ref:`west-building-cmake-config` for
more information on setting CMake arguments.

See :ref:`west-runner` below for more information on the ``runner``
library used by West. The list of runners which support flashing can
be obtained with ``west flash -H``; if run from a build directory or
with ``--build-dir``, this will print additional information on
available runners for your board.

Configuration Overrides
=======================

The CMake cache contains default values West uses while flashing, such
as where the board directory is on the file system, the path to the
zephyr binaries to flash in several formats, and more. You can
override any of this configuration at runtime with additional options.

For example, to override the HEX file containing the Zephyr image to
flash (assuming your runner expects a HEX file), but keep other
flash configuration at default values::

  west flash --hex-file path/to/some/other.hex

The ``west flash -h`` output includes a complete list of overrides
supported by all runners.

Runner-Specific Overrides
=========================

Each runner may support additional options related to flashing. For
example, some runners support an ``--erase`` flag, which mass-erases
the flash storage on your board before flashing the Zephyr image.

To view all of the available options for the runners your board
supports, as well as their usage information, use ``--context`` (or
``-H``)::

  west flash --context

.. important::

   Note the capital H in the short option name. This re-runs the build
   in order to ensure the information displayed is up to date!

When running West outside of a build directory, ``west flash -H`` just
prints a list of runners. You can use ``west flash -H -r
<runner-name>`` to print usage information for options supported by
that runner.

For example, to print usage information about the ``jlink`` runner::

  west flash -H -r jlink

.. _west-debugging:

Debugging: ``west debug``, ``west debugserver``
***********************************************

.. tip::

   Run ``west debug -h`` or ``west debugserver -h`` for additional help.

Basics
======

From a Zephyr build directory, to attach a debugger to your board and
open up a debug console (e.g. a GDB session)::

  west debug

To attach a debugger to your board and open up a local network port
you can connect a debugger to (e.g. an IDE debugger)::

  west debugserver

Without options, the behavior is the same as ``ninja debug`` and
``ninja debugserver`` (or ``make debug``, etc.).

To specify the build directory, use ``--build-dir`` (or ``-d``)::

  west debug --build-dir path/to/build/directory
  west debugserver --build-dir path/to/build/directory

If you don't specify the build directory, these commands search for one in
:file:`build`, then the current working directory. If you set the
``build.dir-fmt`` configuration option (see :ref:`west-building-dirs`), ``west
debug`` searches there instead of :file:`build`.

Choosing a Runner
=================

If your board's Zephyr integration supports debugging with multiple
programs, you can specify which one to use using the ``--runner`` (or
``-r``) option. For example, if West debugs your board with
``pyocd-gdbserver`` by default, but it also supports JLink, you can
override the default with::

  west debug --runner jlink
  west debugserver --runner jlink

See :ref:`west-runner` below for more information on the ``runner``
library used by West. The list of runners which support debugging can
be obtained with ``west debug -H``; if run from a build directory or
with ``--build-dir``, this will print additional information on
available runners for your board.

Configuration Overrides
=======================

The CMake cache contains default values West uses for debugging, such
as where the board directory is on the file system, the path to the
zephyr binaries containing symbol tables, and more. You can override
any of this configuration at runtime with additional options.

For example, to override the ELF file containing the Zephyr binary and
symbol tables (assuming your runner expects an ELF file), but keep
other debug configuration at default values::

  west debug --elf-file path/to/some/other.elf
  west debugserver --elf-file path/to/some/other.elf

The ``west debug -h`` output includes a complete list of overrides
supported by all runners.

Runner-Specific Overrides
=========================

Each runner may support additional options related to debugging. For
example, some runners support flags which allow you to set the network
ports used by debug servers.

To view all of the available options for the runners your board
supports, as well as their usage information, use ``--context`` (or
``-H``)::

  west debug --context

(The command ``west debugserver --context`` will print the same output.)

.. important::

   Note the capital H in the short option name. This re-runs the build
   in order to ensure the information displayed is up to date!

When running West outside of a build directory, ``west debug -H`` just
prints a list of runners. You can use ``west debug -H -r
<runner-name>`` to print usage information for options supported by
that runner.

For example, to print usage information about the ``jlink`` runner::

  west debug -H -r jlink

.. _west-runner:

Flash and debug runners
***********************

The flash and debug commands use Python wrappers around various
:ref:`flash-debug-host-tools`. These wrappers are all defined in a Python
library at :zephyr_file:`scripts/west_commands/runners`. Each wrapper is
called a *runner*. Runners can flash and/or debug Zephyr programs.

The central abstraction within this library is ``ZephyrBinaryRunner``, an
abstract class which represents runners. The set of available runners is
determined by the imported subclasses of ``ZephyrBinaryRunner``.
``ZephyrBinaryRunner`` is available in the ``runners.core`` module; individual
runner implementations are in other submodules, such as ``runners.nrfjprog``,
``runners.openocd``, etc.

Hacking
*******

This section documents the ``runners.core`` module used by the
flash and debug commands. This is the core abstraction used to implement
support for these features.

.. warning::

   These APIs are provided for reference, but they are more "shared code" used
   to implement multiple extension commands than a stable API.

Developers can add support for new ways to flash and debug Zephyr programs by
implementing additional runners. To get this support into upstream Zephyr, the
runner should be added into a new or existing ``runners`` module, and imported
from :file:`runners/__init__.py`.

.. note::

   The test cases in :zephyr_file:`scripts/west_commands/tests` add unit test
   coverage for the runners package and individual runner classes.

   Please try to add tests when adding new runners. Note that if your
   changes break existing test cases, CI testing on upstream pull
   requests will fail.

.. automodule:: runners.core
   :members:

Doing it By Hand
****************

If you prefer not to use West to flash or debug your board, simply
inspect the build directory for the binaries output by the build
system. These will be named something like ``zephyr/zephyr.elf``,
``zephyr/zephyr.hex``, etc., depending on your board's build system
integration. These binaries may be flashed to a board using
alternative tools of your choice, or used for debugging as needed,
e.g. as a source of symbol tables.

By default, these West commands rebuild binaries before flashing and
debugging. This can of course also be accomplished using the usual
targets provided by Zephyr's build system (in fact, that's how these
commands do it).

.. rubric:: Footnotes

.. _cmake(1):
   https://cmake.org/cmake/help/latest/manual/cmake.1.html

.. _CMAKE_VERBOSE_MAKEFILE:
   https://cmake.org/cmake/help/latest/variable/CMAKE_VERBOSE_MAKEFILE.html

.. _CMake Generator:
   https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html
