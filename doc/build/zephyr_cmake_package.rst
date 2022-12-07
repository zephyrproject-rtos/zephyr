.. _cmake_pkg:

Zephyr CMake Package
####################

The Zephyr `CMake package`_ is a convenient way to create a Zephyr-based application.

.. note::
   The :ref:`zephyr-app-types` section introduces the application types
   used in this page.

The Zephyr CMake package ensures that CMake can automatically select a Zephyr installation to use for building
the application, whether it is a :ref:`Zephyr repository application <zephyr-repo-app>`,
a :ref:`Zephyr workspace application <zephyr-workspace-app>`, or a
:ref:`Zephyr freestanding application <zephyr-freestanding-app>`.

When developing a Zephyr-based application, then a developer simply needs to write
``find_package(Zephyr)`` in the beginning of the application :file:`CMakeLists.txt` file.

To use the Zephyr CMake package it must first be exported to the `CMake user package registry`_.
This is means creating a reference to the current Zephyr installation inside the
CMake user package registry.


.. tabs::

   .. group-tab:: Ubuntu

      In Linux, the CMake user package registry is found in:

      ``~/.cmake/packages/Zephyr``

   .. group-tab:: macOS

      In macOS, the CMake user package registry is found in:

      ``~/.cmake/packages/Zephyr``

   .. group-tab:: Windows

      In Windows, the CMake user package registry is found in:

      ``HKEY_CURRENT_USER\Software\Kitware\CMake\Packages\Zephyr``


The Zephyr CMake package allows CMake to automatically find a Zephyr base.
One or more Zephyr installations must be exported.
Exporting multiple Zephyr installations may be useful when developing or testing
Zephyr freestanding applications, Zephyr workspace application with vendor forks, etc..


Zephyr CMake package export (west)
**********************************

When installing Zephyr using :ref:`west <get_the_code>` then it is recommended
to export Zephyr using ``west zephyr-export``.

.. _zephyr_cmake_package_export:

Zephyr CMake package export (without west)
******************************************

Zephyr CMake package is exported to the CMake user package registry using the following commands:

.. code-block:: bash

   cmake -P <PATH-TO-ZEPHYR>/share/zephyr-package/cmake/zephyr_export.cmake

This will export the current Zephyr to the CMake user package registry.

To also export the Zephyr Unittest CMake package, run the following command in addition:

.. code-block:: bash

   cmake -P <PATH-TO-ZEPHYR>/share/zephyrunittest-package/cmake/zephyr_export.cmake

.. _zephyr_cmake_package_zephyr_base:

Zephyr Base Environment Setting
*******************************

The Zephyr CMake package search functionality allows for explicitly specifying
a Zephyr base using an environment variable.

To do this, use the following ``find_package()`` syntax:

.. code-block:: cmake

   find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

This syntax instructs CMake to first search for Zephyr using the Zephyr base environment setting
:envvar:`ZEPHYR_BASE` and then use the normal search paths.

.. _zephyr_cmake_search_order:

Zephyr CMake Package Search Order
*********************************

When Zephyr base environment setting is not used for searching, the Zephyr installation matching
the following criteria will be used:

* A Zephyr repository application will use the Zephyr in which it is located.
  For example:

  .. code-block:: none

        <projects>/zephyr-workspace/zephyr
        └── samples
            └── hello_world

  in this example, ``hello_world`` will use ``<projects>/zephyr-workspace/zephyr``.


* Zephyr workspace application will use the Zephyr that share the same workspace.
  For example:

  .. code-block:: none

     <projects>/zephyr-workspace
     ├── zephyr
     ├── ...
     └── my_applications
          └── my_first_app

  in this example, ``my_first_app`` will use ``<projects>/zephyr-workspace/zephyr`` as this Zephyr
  is located in the same workspace as the Zephyr workspace application.

.. note::
   The root of a Zephyr workspace is identical to ``west topdir`` if the workspace was
   installed using ``west``

* Zephyr freestanding application will use the Zephyr registered in the CMake user package registry.
  For example:

  .. code-block:: none

     <projects>/zephyr-workspace-1
     └── zephyr                       (Not exported to CMake)

     <projects>/zephyr-workspace-2
     └── zephyr                       (Exported to CMake)

     <home>/app
     ├── CMakeLists.txt
     ├── prj.conf
     └── src
         └── main.c

  in this example, only ``<projects>/zephyr-workspace-2/zephyr`` is exported to the CMake package
  registry and therefore this Zephyr will be used by the Zephyr freestanding application
  ``<home>/app``.

  If user wants to test the application with ``<projects>/zephyr-workspace-1/zephyr``, this can be
  done by using the Zephyr Base environment setting, meaning set
  ``ZEPHYR_BASE=<projects>/zephyr-workspace-1/zephyr``, before
  running CMake.

  .. note::

     The Zephyr package selected on the first CMake invocation will be used for all subsequent
     builds. To change the Zephyr package, for example to test the application using Zephyr base
     environment setting, then it is necessary to do a pristine build first
     (See :ref:`application_rebuild`).

Zephyr CMake Package Version
****************************

When writing an application then it is possible to specify a Zephyr version number ``x.y.z`` that
must be used in order to build the application.

Specifying a version is especially useful for a Zephyr freestanding application as it ensures the
application is built with a minimal Zephyr version.

It also helps CMake to select the correct Zephyr to use for building, when there are multiple
Zephyr installations in the system.

For example:

  .. code-block:: cmake

     find_package(Zephyr 2.2.0)
     project(app)

will require ``app`` to be built with Zephyr 2.2.0 as minimum.
CMake will search all exported candidates to find a Zephyr installation which matches this version
criteria.

Thus it is possible to have multiple Zephyr installations and have CMake automatically select
between them based on the version number provided, see `CMake package version`_ for details.

For example:

.. code-block:: none

   <projects>/zephyr-workspace-2.a
   └── zephyr                       (Exported to CMake)

   <projects>/zephyr-workspace-2.b
   └── zephyr                       (Exported to CMake)

   <home>/app
   ├── CMakeLists.txt
   ├── prj.conf
   └── src
       └── main.c

in this case, there are two released versions of Zephyr installed at their own workspaces.
Workspace 2.a and 2.b, corresponding to the Zephyr version.

To ensure ``app`` is built with minimum version ``2.a`` the following ``find_package``
syntax may be used:

.. code-block:: cmake

   find_package(Zephyr 2.a)
   project(app)


Note that both ``2.a`` and ``2.b`` fulfill this requirement.

CMake also supports the keyword ``EXACT``, to ensure an exact version is used, if that is required.
In this case, the application CMakeLists.txt could be written as:

.. code-block:: cmake

   find_package(Zephyr 2.a EXACT)
   project(app)

In case no Zephyr is found which satisfies the version required, as example, the application specifies

.. code-block:: cmake

   find_package(Zephyr 2.z)
   project(app)

then an error similar to below will be printed::

  Could not find a configuration file for package "Zephyr" that is compatible
  with requested version "2.z".

  The following configuration files were considered but not accepted:

    <projects>/zephyr-workspace-2.a/zephyr/share/zephyr-package/cmake/ZephyrConfig.cmake, version: 2.a.0
    <projects>/zephyr-workspace-2.b/zephyr/share/zephyr-package/cmake/ZephyrConfig.cmake, version: 2.b.0


.. note:: It can also be beneficial to specify a version number for Zephyr repository applications
          and Zephyr workspace applications. Specifying a version in those cases ensures the
	  application will only build if the Zephyr repository or workspace is matching.
	  This can be useful to avoid accidental builds when only part of a workspace has been
	  updated.


Multiple Zephyr Installations (Zephyr workspace)
************************************************

Testing out a new Zephyr version, while at the same time keeping the existing Zephyr in the
workspace untouched is sometimes beneficial.

Or having both an upstream Zephyr, Vendor specific, and a custom Zephyr in same workspace.

For example:

.. code-block:: none

   <projects>/zephyr-workspace
   ├── zephyr
   ├── zephyr-vendor
   ├── zephyr-custom
   ├── ...
   └── my_applications
        └── my_first_app


in this setup, ``find_package(Zephyr)`` has the following order of precedence for selecting
which Zephyr to use:

* Project name: ``zephyr``
* First project, when Zephyr projects are ordered lexicographical, in this case.

  * ``zephyr-custom``
  * ``zephyr-vendor``

This means that ``my_first_app`` will use ``<projects>/zephyr-workspace/zephyr``.

It is possible to specify a Zephyr preference list in the application.

A Zephyr preference list can be specified as:

.. code-block:: cmake

   set(ZEPHYR_PREFER "zephyr-custom" "zephyr-vendor")
   find_package(Zephyr)

   project(my_first_app)


the ``ZEPHYR_PREFER`` is a list, allowing for multiple Zephyrs.
If a Zephyr is specified in the list, but not found in the system, it is simply ignored and
``find_package(Zephyr)`` will continue to the next candidate.


This allows for temporary creation of a new Zephyr release to be tested, without touching current
Zephyr. When testing is done, the ``zephyr-test`` folder can simply be removed.
Such a CMakeLists.txt could look as:

.. code-block:: cmake

   set(ZEPHYR_PREFER "zephyr-test")
   find_package(Zephyr)

   project(my_first_app)

.. _cmake_build_config_package:

Zephyr Build Configuration CMake package
****************************************

The Zephyr Build Configuration CMake package provides a possibility for a Zephyr based project to
control Zephyr build settings in a generic way.

It is similar to the per-user ``.zephyrrc`` file that can be used to set :ref:`env_vars`, but it
sets CMake variables instead. It also allows you to automatically share the build configuration
among all users through the project repository. It also allows more advanced use cases, such as loading
of additional CMake boilerplate code.

The Zephyr Build Configuration CMake package will be loaded in the Zephyr boilerplate code after
initial properties and ``ZEPHYR_BASE`` has been defined, but before CMake code execution.
This allows the Zephyr Build Configuration CMake package to setup or extend properties such as:
``DTS_ROOT``, ``BOARD_ROOT``, ``TOOLCHAIN_ROOT`` / other toolchain setup, fixed overlays, and any
other property that can be controlled. It also allows inclusion of additional boilerplate code.

To provide a Zephyr Build Configuration CMake package, create ``ZephyrBuildConfig.cmake`` and place
it in a Zephyr workspace top-level folder as:

.. code-block:: none

   <projects>/zephyr-workspace
   ├── zephyr
   ├── ...
   └── zephyr application (can be named anything)
        └── share/zephyrbuild-package/cmake/ZephyrBuildConfig.cmake

The Zephyr Build Configuration CMake package will not search in any CMake default search paths, and
thus cannot be installed in the CMake package registry. There will be no version checking on the
Zephyr Build Configuration package.

.. note:: ``share/zephyrbuild-package/cmake/ZephyrBuildConfig.cmake`` follows the same folder
          structure as the Zephyr CMake package.

          It is possible to place ``ZephyrBuildConfig.cmake`` directly in a
	  ``<zephyr application>/cmake`` folder or another folder, as long as that folder is
	  honoring the `CMake package search`_ algorithm.

A sample ``ZephyrBuildConfig.cmake`` can be seen below.

.. code-block:: cmake

   # ZephyrBuildConfig.cmake sample code

   # To ensure final path is absolute and does not contain ../.. in variable.
   get_filename_component(APPLICATION_PROJECT_DIR
                          ${CMAKE_CURRENT_LIST_DIR}/../../..
                          ABSOLUTE
   )

   # Add this project to list of board roots
   list(APPEND BOARD_ROOT ${APPLICATION_PROJECT_DIR})

   # Default to GNU Arm Embedded toolchain if no toolchain is set
   if(NOT ENV{ZEPHYR_TOOLCHAIN_VARIANT})
       set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb)
       find_program(GNU_ARM_GCC arm-none-eabi-gcc)
       if(NOT ${GNU_ARM_GCC} STREQUAL GNU_ARM_GCC-NOTFOUND)
           # The toolchain root is located above the path to the compiler.
           get_filename_component(GNUARMEMB_TOOLCHAIN_PATH ${GNU_ARM_GCC}/../.. ABSOLUTE)
       endif()
   endif()

Zephyr Build Configuration CMake package (Freestanding application)
*******************************************************************

The Zephyr Build Configuration CMake package can be located outside a Zephyr
workspace, for example located with a :ref:`zephyr-freestanding-app`.

Create the build configuration as described in the previous section, and then
refer to the location of your Zephyr Build Configuration CMake package using
the CMake variable ``ZephyrBuildConfiguration_ROOT``.

#. At the CMake command line, like this:

   .. code-block:: console

      cmake -DZephyrBuildConfiguration_ROOT=<path-to-build-config> ...

#. At the top of your application's top level :file:`CMakeLists.txt`, like this:

   .. code-block:: cmake

      set(ZephyrBuildConfiguration_ROOT <path-to-build-config>)
      find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

   If you choose this option, make sure to set the variable **before**  calling
   ``find_package(Zephyr ...)``, as shown above.

#. In a separate CMake script which is pre-loaded to populate the CMake cache,
   like this:

   .. code-block:: cmake

      # Put this in a file with a name like "zephyr-settings.cmake"
      set(ZephyrBuildConfiguration_ROOT <path-to-build-config>
          CACHE STRING "pre-cached build config"
      )

   You can tell the build system to use this file by adding ``-C
   zephyr-settings.cmake`` to your CMake command line.
   This principle is useful when not using ``west`` as both this setting and
   Zephyr modules can be specified using the same file.
   See Zephyr module :ref:`modules_without_west`.

Zephyr CMake package source code
********************************

The Zephyr CMake package source code in
:zephyr_file:`share/zephyr-package/cmake` and
:zephyr_file:`share/zephyrunittest-package/cmake` contains the CMake config
package which is used by the CMake ``find_package`` function.

It also contains code for exporting Zephyr as a CMake config package.

The following is an overview of the files in these directories:

:file:`ZephyrConfigVersion.cmake`
    The Zephyr package version file. This file is called by CMake to determine
    if this installation fulfils the requirements specified by user when calling
    ``find_package(Zephyr ...)``. It is also responsible for detection of Zephyr
    repository or workspace only installations.

:file:`ZephyrUnittestConfigVersion.cmake`
    Same responsibility as ``ZephyrConfigVersion.cmake``, but for unit tests.
    Includes ``ZephyrConfigVersion.cmake``.

:file:`ZephyrConfig.cmake`
    The Zephyr package file. This file is called by CMake to for the package
    meeting which fulfils the requirements specified by user when calling
    ``find_package(Zephyr ...)``. This file is responsible for sourcing of
    boilerplate code.

:file:`ZephyrUnittestConfig.cmake`
    Same responsibility as ``ZephyrConfig.cmake``, but for unit tests.
    Includes ``ZephyrConfig.cmake``.

:file:`zephyr_package_search.cmake`
   Common file used for detection of Zephyr repository and workspace candidates.
   Used by ``ZephyrConfigVersion.cmake`` and ``ZephyrConfig.cmake`` for common code.

:file:`zephyr_export.cmake`
   See :ref:`zephyr_cmake_package_export`.

.. _CMake package: https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html
.. _CMake user package registry: https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html#user-package-registry
.. _CMake package version: https://cmake.org/cmake/help/latest/command/find_package.html#version-selection
.. _CMake package search: https://cmake.org/cmake/help/latest/command/find_package.html#search-procedure
