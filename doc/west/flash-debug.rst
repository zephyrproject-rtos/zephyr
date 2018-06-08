.. _west-flash-debug:

West: Flashing and Debugging
############################

West provides three commands for running and interacting with Zephyr
programs running on a board: ``flash``, ``debug``, and
``debugserver``.

These use information stored in the CMake cache [#cmakecache]_ to
flash or attach a debugger to a board supported by Zephyr. The CMake
build system commands with the same names directly delegate to West.

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

Library Backend: ``west.runner``
********************************

In keeping with West's :ref:`west-design-constraints`, the flash and
debug commands are wrappers around a separate library that is part of
West, and can be used by other code.

This library is the ``west.runner`` subpackage of West itself.  The
central abstraction within this library is ``ZephyrBinaryRunner``, an
abstract class which represents *runner* objects, which can flash
and/or debug Zephyr programs. The set of available runners is
determined by the imported subclasses of ``ZephyrBinaryRunner``.
``ZephyrBinaryRunner`` is available in the ``west.runner.core``
module; individual runner implementations are in other submodules,
such as ``west.runner.nrfjprog``, ``west.runner.openocd``, etc.

Developers can add support for new ways to flash and debug Zephyr
programs by implementing additional runners. To get this support into
upstream Zephyr, the runner should be added into a new or existing
``west.runner`` module, and imported from
:file:`west/runner/__init__.py`.

.. important::

   Submit any changes to West to its own separate Git repository
   (https://github.com/zephyrproject-rtos/west), not to the copy of
   West currently present in the Zephyr tree. This copy is a temporary
   measure; when West learns how to manage multiple repositories, the
   copy will be removed.

API documentation for the core module follows.

.. automodule:: west.runner.core
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
targets provided by Zephyr's build system (in fact, that's how West
does it).

.. rubric:: Footnotes

.. [#cmakecache]

   The CMake cache is a file containing saved variables and values
   which is created by CMake when it is first run to generate a build
   system. See the `cmake(1)`_ manual for more details.

.. _cmake(1):
   https://cmake.org/cmake/help/latest/manual/cmake.1.html
