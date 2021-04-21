.. _west-zephyr-ext-cmds:

Additional Zephyr extension commands
####################################

This page documents miscellaneous :ref:`west-zephyr-extensions`.

.. _west-boards:

Listing boards: ``west boards``
*******************************

The ``boards`` command can be used to list the boards that are supported by
Zephyr without having to resort to additional sources of information.

It can be run by typing::

  west boards

This command lists all supported boards in a default format. If you prefer to
specify the display format yourself you can use the ``--format`` (or ``-f``)
flag::

  west boards -f "{arch}:{name}"

Additional help about the formatting options can be found by running::

  west boards -h

.. _west-zephyr-export:

Installing CMake packages: ``west zephyr-export``
*************************************************

This command registers the current Zephyr installation as a CMake
config package in the CMake user package registry.

In Windows, the CMake user package registry is found in
``HKEY_CURRENT_USER\Software\Kitware\CMake\Packages``.

In Linux and MacOS, the CMake user package registry is found in.
:file:`~/.cmake/packages`.

You may run this command when setting up a Zephyr workspace. If you do,
application CMakeLists.txt files that are outside of your workspace will be
able to find the Zephyr repository with the following:

.. code-block:: cmake

   find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

See :zephyr_file:`share/zephyr-package/cmake` for details.
