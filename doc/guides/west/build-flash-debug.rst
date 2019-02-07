.. _west-build-flash-debug:

Building, Flashing and Debugging
################################

West provides 5 commands for building, flashing, and interacting with Zephyr
programs running on a board: ``build``, ``flash``, ``debug``, ``debugserver``
and ``attach``.

These use information stored in the CMake cache [#cmakecache]_ to
flash or attach a debugger to a board supported by Zephyr. The exception is
starting a clean build (i.e. with no previous artifacts) which will in fact
run CMake thus creating the corresponding cache.
The CMake build system commands with the same names (i.e. all but ``build``)
directly delegate to West.

.. Add a per-page contents at the top of the page. This page is nested
   deeply enough that it doesn't have any subheadings in the main nav.

.. only:: html

   .. contents::
      :local:

.. _west-building:

Building: ``west build``
************************

.. tip:: Run ``west build -h`` for additional help.

The ``build`` command allows you to build any source tree from any directory
in your file system, placing the build results in a folder of your choice.

In its simplest form, the command can be run by navigating to the root folder
(i.e. the folder containing a file:`CMakeLists.txt` file) of the Zephyr
application of your choice and running::

  west build -b <BOARD>

Where ``<BOARD>`` is the name of the board you want to build for. This is
exactly the same name you would supply to CMake if you were to invoke it with:
``cmake -DBOARD=<BOARD>``.
A build folder named :file:`build` (default name) will be created and the
build output will be placed in it.

To specify the build directory, use ``--build-dir`` (or ``-d``)::

  west build -b <BOARD> --build-dir path/to/build/directory

Since the build directory defaults to :file:`build`, if you do not specify
a build directory but a folder named file:`build` is present, that will be used,
allowing you to incrementally build from outside the file:`build` folder with
no additional parameters.

.. note::
  The ``-b <BOARD>`` parameter is only required when there is no CMake cache
  present at all, usually when you are building from scratch or creating a
  brand new build directory. After that you can rebuild (even with additional
  parameters) without having to specify the board again. If you're unsure
  whether ``-b`` is required, just try leaving it out. West will print an
  error if the option is required and was not given.

To specify the source directory, use ``--source-dir`` (or ``-s``)::

  west build -b <BOARD> --source-dir path/to/source/directory

Additionally you can specify the build system target using the ``--target``
(or ``-t``) option. For example, to run the ``clean`` target::

  west build -t clean

You can list all targets with ``ninja help`` (or ``west build -t help``) inside
the build folder.


Finally, you can add additional arguments to the CMake invocation performed by
``west build`` by supplying additional parameters (after a ``--``) to the
command. For example, to use the Unix Makefiles CMake generator instead of
Ninja (which ``west build`` uses by default), run::

  west build -- -G'Unix Makefiles'

As another example, the following command adds the ``file.conf`` Kconfig
fragment to the files which are merged into your final build configuration::

  west build -- -DOVERLAY_CONFIG=file.conf

Passing additional CMake arguments like this forces ``west build`` to re-run
CMake, even if a build system has already been generated. You can also force
a CMake re-run using the ``-c`` (or ``--cmake``) option::

  west build -c

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

Since the build directory defaults to :file:`build`, if you do not specify
a build directory but a folder named :file:`build` is present, that will be
used, allowing you to flash from outside the file:`build` folder with no
additional parameters.

Choosing a Runner
=================

If your board's Zephyr integration supports flashing with multiple
programs, you can specify which one to use using the ``--runner`` (or
``-r``) option. For example, if West flashes your board with
``nrfjprog`` by default, but it also supports JLink, you can override
the default with::

  west flash --runner jlink

See :ref:`west-runner` below for more information on the ``runner``
library used by West. The list of runners which support flashing can
be obtained with ``west flash -H``; if run from a build directory or
with ``--build-dir``, this will print additional information on
available runners for your board.

Configuration Overrides
=======================

The CMake cache contains default values West uses while flashing, such
as where the board directory is on the file system, the path to the
kernel binaries to flash in several formats, and more. You can
override any of this configuration at runtime with additional options.

For example, to override the HEX file containing the Zephyr image to
flash (assuming your runner expects a HEX file), but keep other
flash configuration at default values::

  west flash --kernel-hex path/to/some/other.hex

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

Since the build directory defaults to :file:`build`, if you do not specify
a build directory but a folder named :file:`build` is present, that will be
used, allowing you to debug from outside the file:`build` folder with no
additional parameters.

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
kernel binaries containing symbol tables, and more. You can override
any of this configuration at runtime with additional options.

For example, to override the ELF file containing the Zephyr binary and
symbol tables (assuming your runner expects an ELF file), but keep
other debug configuration at default values::

  west debug --kernel-elf path/to/some/other.elf
  west debugserver --kernel-elf path/to/some/other.elf

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

Implementation Details
**********************

The flash and debug commands are implemented as west *extension
commands*: that is, they are west commands whose source code lives
outside the west repository. Some reasons this choice was made are:

- Their implementations are tightly coupled to the Zephyr build
  system, e.g. due to their reliance on CMake cache variables.

- Pull requests adding features to them are almost always motivated by
  a corresponding change to an upstream board, so it makes sense to
  put them in Zephyr to avoid needing pull requests in multiple
  repositories.

- Many users find it natural to search for their implementations in
  the Zephyr source tree.

The extension commands are a thin wrapper around a package called
``runners`` (this package is also in the Zephyr tree, in
:file:`scripts/west_commands/runners`).

The central abstraction within this library is ``ZephyrBinaryRunner``,
an abstract class which represents *runner* objects, which can flash
and/or debug Zephyr programs. The set of available runners is
determined by the imported subclasses of ``ZephyrBinaryRunner``.
``ZephyrBinaryRunner`` is available in the ``runners.core`` module;
individual runner implementations are in other submodules, such as
``runners.nrfjprog``, ``runners.openocd``, etc.

Hacking and APIs
****************

Developers can add support for new ways to flash and debug Zephyr
programs by implementing additional runners. To get this support into
upstream Zephyr, the runner should be added into a new or existing
``runners`` module, and imported from :file:`runner/__init__.py`.

.. note::

   The test cases in :file:`scripts/west_commands/tests` add unit test
   coverage for the runners package and individual runner classes.

   Please try to add tests when adding new runners. Note that if your
   changes break existing test cases, CI testing on upstream pull
   requests will fail.

API Documentation for the ``runners.core`` module can be found in
:ref:`west-apis`.

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

.. [#cmakecache]

   The CMake cache is a file containing saved variables and values
   which is created by CMake when it is first run to generate a build
   system. See the `cmake(1)`_ manual for more details.

.. _cmake(1):
   https://cmake.org/cmake/help/latest/manual/cmake.1.html

.. _namespace package:
   https://www.python.org/dev/peps/pep-0420/
