.. _application:

Application Development
#######################

.. note::

   In this document, we'll assume:

   - your **application directory**, :file:`<app>`, is something like :file:`<home>/zephyrproject/app`
   - its **build directory** is :file:`<app>/build`

   These terms are defined below. On Linux/macOS, <home> is equivalent to
   ``~``. On Windows, it's ``%userprofile%``.

   Keeping your application inside the workspace (:file:`<home>/zephyrproject`)
   makes it easier to use ``west build`` and other commands with it. (You can
   put your application anywhere as long as :ref:`ZEPHYR_BASE
   <important-build-vars>` is set appropriately, though.)

Overview
********

Zephyr's build system is based on `CMake`_.

The build system is application-centric, and requires Zephyr-based applications
to initiate building the Zephyr source code. The application build controls
the configuration and build process of both the application and Zephyr itself,
compiling them into a single binary.

The main zephyr repository contains Zephyr's source code, configuration files,
and build system. You also likely have installed various :ref:`modules`
alongside the zephyr repository, which provide third party source code
integration.

The files in the **application directory** link Zephyr and any modules with the
application. This directory contains all application-specific files, such as
application-specific configuration files and source code.

Here are the files in a simple Zephyr application:

.. code-block:: none

   <app>
   ├── CMakeLists.txt
   ├── app.overlay
   ├── prj.conf
   └── src
       └── main.c

These contents are:

* **CMakeLists.txt**: This file tells the build system where to find the other
  application files, and links the application directory with Zephyr's CMake
  build system. This link provides features supported by Zephyr's build system,
  such as board-specific configuration files, the ability to run and
  debug compiled binaries on real or emulated hardware, and more.

* **app.overlay**: This is a devicetree overlay file that specifies
  application-specific changes which should be applied to the base devicetree
  for any board you build for. The purpose of devicetree overlays is
  usually to configure something about the hardware used by the application.

  The build system looks for :file:`app.overlay` by default, but you can add
  more devicetree overlays, and other default files are also searched for.

  See :ref:`devicetree` for more information about devicetree.

* **prj.conf**: This is a Kconfig fragment that specifies application-specific
  values for one or more Kconfig options. These application settings are merged
  with other settings to produce the final configuration. The purpose of
  Kconfig fragments is usually to configure the software features used by
  the application.

  The build system looks for :file:`prj.conf` by default, but you can add more
  Kconfig fragments, and other default files are also searched for.

  See :ref:`application-kconfig` below for more information.

* **main.c**: A source code file. Applications typically contain source files
  written in C, C++, or assembly language. The Zephyr convention is to place
  them in a subdirectory of :file:`<app>` named :file:`src`.

Once an application has been defined, you will use CMake to generate a **build
directory**, which contains the files you need to build the application and
Zephyr, then link them together into a final binary you can run on your board.
The easiest way to do this is with :ref:`west build <west-building>`, but you
can use CMake directly also. Application build artifacts are always generated
in a separate build directory: Zephyr does not support "in-tree" builds.

The following sections describe how to create, build, and run Zephyr
applications, followed by more detailed reference material.

.. _zephyr-app-types:

Application types
*****************

We distinguish three basic types of Zephyr application based on where
:file:`<app>` is located:

.. table::

   +------------------------------+--------------------------------+
   | Application type             | :file:`<app>` location         |
   +------------------------------+--------------------------------+
   | :ref:`repository             | zephyr repository              |
   | <zephyr-repo-app>`           |                                |
   +------------------------------+--------------------------------+
   | :ref:`workspace              | west workspace where Zephyr is |
   | <zephyr-workspace-app>`      | installed                      |
   +------------------------------+--------------------------------+
   | :ref:`freestanding           | other locations                |
   | <zephyr-freestanding-app>`   |                                |
   +------------------------------+--------------------------------+

We'll discuss these more below. To learn how the build system supports each
type, see :ref:`cmake_pkg`.

.. _zephyr-repo-app:

Zephyr repository application
=============================

An application located within the ``zephyr`` source code repository in a Zephyr
:ref:`west workspace <west-workspaces>` is referred to as a Zephyr repository
application. In the following example, the :ref:`hello_world sample
<hello_world>` is a Zephyr repository application:

.. code-block:: none

   zephyrproject/
   ├─── .west/
   │    └─── config
   └─── zephyr/
        ├── arch/
        ├── boards/
        ├── cmake/
        ├── samples/
        │    ├── hello_world/
        │    └── ...
        ├── tests/
        └── ...

.. _zephyr-workspace-app:

Zephyr workspace application
============================

An application located within a :ref:`workspace <west-workspaces>`, but outside
the zephyr repository itself, is referred to as a Zephyr workspace application.
In the following example, ``app`` is a Zephyr workspace application:

.. code-block:: none

   zephyrproject/
   ├─── .west/
   │    └─── config
   ├─── zephyr/
   ├─── bootloader/
   ├─── modules/
   ├─── tools/
   ├─── <vendor/private-repositories>/
   └─── applications/
        └── app/

.. _zephyr-freestanding-app:

Zephyr freestanding application
===============================

A Zephyr application located outside of a Zephyr :ref:`workspace
<west-workspaces>` is referred to as a Zephyr freestanding application. In the
following example, ``app`` is a Zephyr freestanding application:

.. code-block:: none

   <home>/
   ├─── zephyrproject/
   │     ├─── .west/
   │     │    └─── config
   │     ├── zephyr/
   │     ├── bootloader/
   │     ├── modules/
   │     └── ...
   │
   └─── app/
        ├── CMakeLists.txt
        ├── prj.conf
        └── src/
            └── main.c

Creating an Application
***********************

example-application
===================

The `example-application`_ Git repository contains a reference :ref:`workspace
application <zephyr-workspace-app>`. It is recommended to use it as a reference
when creating your own application as described in the following sections.

The example-application repository demonstrates how to use several
commonly-used features, such as:

- Custom :ref:`board ports <board_porting_guide>`
- Custom :ref:`devicetree bindings <dt-bindings>`
- Custom :ref:`device drivers <device_model_api>`
- Continuous Integration (CI) setup, including using :ref:`twister <twister_script>`
- A custom west :ref:`extension command <west-extensions>`

Basic example-application Usage
===============================

The easiest way to get started with the example-application repository within
an existing Zephyr workspace is to follow these steps:

.. code-block:: console

   cd <home>/zephyrproject
   git clone https://github.com/zephyrproject-rtos/example-application my-app

The directory name :file:`my-app` above is arbitrary: change it as needed. You
can now go into this directory and adapt its contents to suit your needs. Since
you are using an existing Zephyr workspace, you can use ``west build`` or any
other west commands to build, flash, and debug.

Advanced example-application Usage
==================================

You can also use the example-application repository as a starting point for
building your own customized Zephyr-based software distribution. This lets you
do things like:

- remove Zephyr modules you don't need
- add additional custom repositories of your own
- override repositories provided by Zephyr with your own versions
- share the results with others and collaborate further

The example-application repository contains a :file:`west.yml` file and is
therefore also a west :ref:`manifest repository <west-workspace>`. Use this to
create a new, customized workspace by following these steps:

.. code-block:: console

   cd <home>
   mkdir my-workspace
   cd my-workspace
   git clone https://github.com/zephyrproject-rtos/example-application my-manifest-repo
   west init -l my-manifest-repo

This will create a new workspace with the :ref:`T2 topology <west-t2>`, with
:file:`my-manifest-repo` as the manifest repository. The :file:`my-workspace`
and :file:`my-manifest-repo` names are arbitrary: change them as needed.

Next, customize the manifest repository. The initial contents of this
repository will match the example-application's contents when you clone it. You
can then edit :file:`my-manifest-repo/west.yml` to your liking, changing the
set of repositories in it as you wish. See :ref:`west-manifest-import` for many
examples of how to add or remove different repositories from your workspace as
needed. Make any other changes you need to other files.

When you are satisfied, you can run:

.. code-block::

   west update

and your workspace will be ready for use.

If you push the resulting :file:`my-manifest-repo` repository somewhere else,
you can share your work with others. For example, let's say you push the
repository to ``https://git.example.com/my-manifest-repo``. Other people can
then set up a matching workspace by running:

.. code-block::

   west init -m https://git.example.com/my-manifest-repo my-workspace
   cd my-workspace
   west update

From now on, you can collaborate on the shared software by pushing changes to
the repositories you are using and updating :file:`my-manifest-repo/west.yml`
as needed to add and remove repositories, or change their contents.

Creating an Application by Hand
===============================

You can follow these steps to create a basic application directory from
scratch. However, using the `example-application`_ repository or one of
Zephyr's :ref:`samples-and-demos` as a starting point is likely to be easier.

#. Create an application directory.

   For example, in a Unix shell or Windows ``cmd.exe`` prompt:

   .. code-block:: console

      mkdir app

   .. warning::

      Building Zephyr or creating an application in a directory with spaces
      anywhere on the path is not supported. So the Windows path
      :file:`C:\\Users\\YourName\\app` will work, but
      :file:`C:\\Users\\Your Name\\app` will not.

#. Create your source code files.

   It's recommended to place all application source code in a subdirectory
   named :file:`src`.  This makes it easier to distinguish between project
   files and sources.

   Continuing the previous example, enter:

   .. code-block:: console

      cd app
      mkdir src

#. Place your application source code in the :file:`src` sub-directory. For
   this example, we'll assume you created a file named :file:`src/main.c`.

#. Create a file named :file:`CMakeLists.txt` in the ``app`` directory with the
   following contents:

   .. code-block:: cmake

      cmake_minimum_required(VERSION 3.20.0)

      find_package(Zephyr)
      project(my_zephyr_app)

      target_sources(app PRIVATE src/main.c)

   Notes:

   - The ``cmake_minimum_required()`` call is required by CMake. It is also
     invoked by the Zephyr package on the next line. CMake will error out if
     its version is older than either the version in your
     :file:`CMakeLists.txt` or the version number in the Zephyr package.

   - ``find_package(Zephyr)`` pulls in the Zephyr build system, which creates a
     CMake target named ``app`` (see :ref:`cmake_pkg`). Adding sources to this
     target is how you include them in the build. The Zephyr package will
     define ``Zephyr-Kernel`` as a CMake project and enable support for the
     ``C``, ``CXX``, ``ASM`` languages.

   - ``project(my_zephyr_app)`` defines your application's CMake
     project.  This must be called after ``find_package(Zephyr)`` to avoid
     interference with Zephyr's ``project(Zephyr-Kernel)``.

   - ``target_sources(app PRIVATE src/main.c)`` is to add your source file to
     the ``app`` target. This must come after ``find_package(Zephyr)`` which
     defines the target. You can add as many files as you want with
     ``target_sources()``.

#. Create at least one Kconfig fragment for your application (usually named
   :file:`prj.conf`) and set Kconfig option values needed by your application
   there. See :ref:`application-kconfig`.

#. Configure any devicetree overlays needed by your application, usually in a
   file named :file:`app.overlay`. See :ref:`set-devicetree-overlays`.

#. Set up any other files you may need, such as :ref:`twister <twister_script>`
   configuration files, continuous integration files, documentation, etc.

.. _important-build-vars:

Important Build System Variables
********************************

You can control the Zephyr build system using many variables. This
section describes the most important ones that every Zephyr developer
should know about.

.. note::

   The variables :makevar:`BOARD`, :makevar:`CONF_FILE`, and
   :makevar:`DTC_OVERLAY_FILE` can be supplied to the build system in
   3 ways (in order of precedence):

   * As a parameter to the ``west build`` or ``cmake`` invocation via the
     ``-D`` command-line switch. If you have multiple overlay files, you should
     use quotations, ``"file1.overlay;file2.overlay"``
   * As :ref:`env_vars`.
   * As a ``set(<VARIABLE> <VALUE>)`` statement in your :file:`CMakeLists.txt`

* :makevar:`ZEPHYR_BASE`: Zephyr base variable used by the build system.
  ``find_package(Zephyr)`` will automatically set this as a cached CMake
  variable. But ``ZEPHYR_BASE`` can also be set as an environment variable in
  order to force CMake to use a specific Zephyr installation.

* :makevar:`BOARD`: Selects the board that the application's build
  will use for the default configuration.  See :ref:`boards` for
  built-in boards, and :ref:`board_porting_guide` for information on
  adding board support.

* :makevar:`CONF_FILE`: Indicates the name of one or more Kconfig configuration
  fragment files. Multiple filenames can be separated with either spaces or
  semicolons. Each file includes Kconfig configuration values that override
  the default configuration values.

  See :ref:`initial-conf` for more information.

* :makevar:`OVERLAY_CONFIG`: Additional Kconfig configuration fragment files.
  Multiple filenames can be separated with either spaces or semicolons. This
  can be useful in order to leave :makevar:`CONF_FILE` at its default value,
  but "mix in" some additional configuration options.

* :makevar:`DTC_OVERLAY_FILE`: One or more devicetree overlay files to use.
  Multiple files can be separated with semicolons.
  See :ref:`set-devicetree-overlays` for examples and :ref:`devicetree-intro`
  for information about devicetree and Zephyr.

* :makevar:`SHIELD`: see :ref:`shields`

* :makevar:`ZEPHYR_MODULES`: A CMake list containing absolute paths of
  additional directories with source code, Kconfig, etc. that should be used in
  the application build. See :ref:`modules` for details. If you set this
  variable, it must be a complete list of all modules to use, as the build
  system will not automatically pick up any modules from west.

* :makevar:`ZEPHYR_EXTRA_MODULES`: Like :makevar:`ZEPHYR_MODULES`, except these
  will be added to the list of modules found via west, instead of replacing it.

.. note::

   You can use a :ref:`cmake_build_config_package` to share common settings for
   these variables.

Application CMakeLists.txt
**************************

Every application must have a :file:`CMakeLists.txt` file. This file is the
entry point, or top level, of the build system. The final :file:`zephyr.elf`
image contains both the application and the kernel libraries.

This section describes some of what you can do in your :file:`CMakeLists.txt`.
Make sure to follow these steps in order.

#. If you only want to build for one board, add the name of the board
   configuration for your application on a new line. For example:

   .. code-block:: cmake

      set(BOARD qemu_x86)

   Refer to :ref:`boards` for more information on available boards.

   The Zephyr build system determines a value for :makevar:`BOARD` by checking
   the following, in order (when a BOARD value is found, CMake stops looking
   further down the list):

   - Any previously used value as determined by the CMake cache takes highest
     precedence. This ensures you don't try to run a build with a different
     :makevar:`BOARD` value than you set during the build configuration step.

   - Any value given on the CMake command line (directly or indirectly via
     ``west build``) using ``-DBOARD=YOUR_BOARD`` will be checked for and
     used next.

   - If an :ref:`environment variable <env_vars>` ``BOARD`` is set, its value
     will then be used.

   - Finally, if you set ``BOARD`` in your application :file:`CMakeLists.txt`
     as described in this step, this value will be used.

#. If your application uses a configuration file or files other than
   the usual :file:`prj.conf` (or :file:`prj_YOUR_BOARD.conf`, where
   ``YOUR_BOARD`` is a board name), add lines setting the
   :makevar:`CONF_FILE` variable to these files appropriately.
   If multiple filenames are given, separate them by a single space or
   semicolon.  CMake lists can be used to build up configuration fragment
   files in a modular way when you want to avoid setting :makevar:`CONF_FILE`
   in a single place. For example:

   .. code-block:: cmake

     set(CONF_FILE "fragment_file1.conf")
     list(APPEND CONF_FILE "fragment_file2.conf")

   See :ref:`initial-conf` for more information.

#. If your application uses devicetree overlays, you may need to set
   :ref:`DTC_OVERLAY_FILE <important-build-vars>`.
   See :ref:`set-devicetree-overlays`.

#. If your application has its own kernel configuration options,
   create a :file:`Kconfig` file in the same directory as your
   application's :file:`CMakeLists.txt`.

   See :ref:`the Kconfig section of the manual <kconfig>` for detailed
   Kconfig documentation.

   An (unlikely) advanced use case would be if your application has its own
   unique configuration **options** that are set differently depending on the
   build configuration.

   If you just want to set application specific **values** for existing Zephyr
   configuration options, refer to the :makevar:`CONF_FILE` description above.

   Structure your :file:`Kconfig` file like this:

   .. literalinclude:: application-kconfig.include

   .. note::

      Environment variables in ``source`` statements are expanded directly, so
      you do not need to define an ``option env="ZEPHYR_BASE"`` Kconfig
      "bounce" symbol. If you use such a symbol, it must have the same name as
      the environment variable.

      See :ref:`kconfig_extensions` for more information.

   The :file:`Kconfig` file is automatically detected when placed in
   the application directory, but it is also possible for it to be
   found elsewhere if the CMake variable :makevar:`KCONFIG_ROOT` is
   set with an absolute path.

#. Specify that the application requires Zephyr on a new line, **after any
   lines added from the steps above**:

   .. code-block:: cmake

      find_package(Zephyr)
      project(my_zephyr_app)

   .. note:: ``find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})`` can be used if
             enforcing a specific Zephyr installation by explicitly
             setting the ``ZEPHYR_BASE`` environment variable should be
             supported. All samples in Zephyr supports the ``ZEPHYR_BASE``
             environment variable.

#. Now add any application source files to the 'app' target
   library, each on their own line, like so:

   .. code-block:: cmake

      target_sources(app PRIVATE src/main.c)

Below is a simple example :file:`CMakeList.txt`:

.. code-block:: cmake

   set(BOARD qemu_x86)

   find_package(Zephyr)
   project(my_zephyr_app)

   target_sources(app PRIVATE src/main.c)

The Cmake property ``HEX_FILES_TO_MERGE``
leverages the application configuration provided by
Kconfig and CMake to let you merge externally built hex files
with the hex file generated when building the Zephyr application.
For example:

.. code-block:: cmake

  set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE
      ${app_bootloader_hex}
      ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
      ${app_provision_hex})

CMakeCache.txt
**************

CMake uses a CMakeCache.txt file as persistent key/value string
storage used to cache values between runs, including compile and build
options and paths to library dependencies. This cache file is created
when CMake is run in an empty build folder.

For more details about the CMakeCache.txt file see the official CMake
documentation `runningcmake`_ .

.. _runningcmake: http://cmake.org/runningcmake/

Application Configuration
*************************

.. _application-configuration-directory:

Application Configuration Directory
===================================

Zephyr will use configuration files from the application's configuration
directory except for files with an absolute path provided by the arguments
described earlier, for example ``CONF_FILE``, ``OVERLAY_CONFIG``, and
``DTC_OVERLAY_FILE``.

The application configuration directory is defined by the
``APPLICATION_CONFIG_DIR`` variable.

``APPLICATION_CONFIG_DIR`` will be set by one of the sources below with the
highest priority listed first.

1. If ``APPLICATION_CONFIG_DIR`` is specified by the user with
   ``-DAPPLICATION_CONFIG_DIR=<path>`` or in a CMake file before
   ``find_package(Zephyr)`` then this folder is used a the application's
   configuration directory.

2. The application's source directory.

.. _application-kconfig:

Kconfig Configuration
=====================

Application configuration options are usually set in :file:`prj.conf` in the
application directory. For example, C++ support could be enabled with this
assignment:

.. code-block:: none

   CONFIG_CPP=y

Looking at :ref:`existing samples <samples-and-demos>` is a good way to get
started.

See :ref:`setting_configuration_values` for detailed documentation on setting
Kconfig configuration values. The :ref:`initial-conf` section on the same page
explains how the initial configuration is derived. See :ref:`kconfig-search`
for a complete list of configuration options.
See :ref:`hardening` for security information related with Kconfig options.

The other pages in the :ref:`Kconfig section of the manual <kconfig>` are also
worth going through, especially if you planning to add new configuration
options.

Experimental features
~~~~~~~~~~~~~~~~~~~~~

Zephyr is a project under constant development and thus there are features that
are still in early stages of their development cycle. Such features will be
marked ``[EXPERIMENTAL]`` in their Kconfig title.

The :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` setting can be used to enable warnings
at CMake configure time if any experimental feature is enabled.

.. code-block:: none

   CONFIG_WARN_EXPERIMENTAL=y

For example, if option ``CONFIG_FOO`` is experimental, then enabling it and
:kconfig:option:`CONFIG_WARN_EXPERIMENTAL` will print the following warning at
CMake configure time when you build an application:

.. code-block:: none

   warning: Experimental symbol FOO is enabled.

Devicetree Overlays
===================

See :ref:`set-devicetree-overlays`.

Application-Specific Code
*************************

Application-specific source code files are normally added to the
application's :file:`src` directory. If the application adds a large
number of files the developer can group them into sub-directories
under :file:`src`, to whatever depth is needed.

Application-specific source code should not use symbol name prefixes that have
been reserved by the kernel for its own use. For more information, see `Naming
Conventions
<https://github.com/zephyrproject-rtos/zephyr/wiki/Naming-Conventions>`_.

Third-party Library Code
========================

It is possible to build library code outside the application's :file:`src`
directory but it is important that both application and library code targets
the same Application Binary Interface (ABI). On most architectures there are
compiler flags that control the ABI targeted, making it important that both
libraries and applications have certain compiler flags in common. It may also
be useful for glue code to have access to Zephyr kernel header files.

To make it easier to integrate third-party components, the Zephyr
build system has defined CMake functions that give application build
scripts access to the zephyr compiler options. The functions are
documented and defined in :zephyr_file:`cmake/extensions.cmake`
and follow the naming convention ``zephyr_get_<type>_<format>``.

The following variables will often need to be exported to the
third-party build system.

* ``CMAKE_C_COMPILER``, ``CMAKE_AR``.

* ``ARCH`` and ``BOARD``, together with several variables that identify the
  Zephyr kernel version.

:zephyr_file:`samples/application_development/external_lib` is a sample
project that demonstrates some of these features.


.. _build_an_application:

Building an Application
***********************

The Zephyr build system compiles and links all components of an application
into a single application image that can be run on simulated hardware or real
hardware.

Like any other CMake-based system, the build process takes place :ref:`in
two stages <cmake-details>`. First, build files (also known as a buildsystem)
are generated using the ``cmake`` command-line tool while specifying a
generator. This generator determines the native build tool the buildsystem
will use in the second stage.
The second stage runs the native build tool to actually build the
source files and generate an image. To learn more about these concepts refer to
the `CMake introduction`_ in the official CMake documentation.

Although the default build tool in Zephyr is :std:ref:`west <west>`, Zephyr's
meta-tool, which invokes ``cmake`` and the underlying build tool (``ninja`` or
``make``) behind the scenes, you can also choose to invoke ``cmake`` directly if
you prefer.  On Linux and macOS you can choose between the ``make`` and
``ninja``
generators (i.e. build tools), whereas on Windows you need to use ``ninja``,
since ``make`` is not supported on this platform.
For simplicity we will use ``ninja`` throughout this guide, and if you
choose to use ``west build`` to build your application know that it will
default to ``ninja`` under the hood.

As an example, let's build the Hello World sample for the ``reel_board``:

.. zephyr-app-commands::
   :tool: all
   :app: samples/hello_world
   :board: reel_board
   :goals: build

On Linux and macOS, you can also build with ``make`` instead of ``ninja``:

Using west:

- to use ``make`` just once, add ``-- -G"Unix Makefiles"`` to the west build
  command line; see the :ref:`west build <west-building-generator>`
  documentation for an example.
- to use ``make`` by default from now on, run ``west config build.generator
  "Unix Makefiles"``.

Using CMake directly:

.. zephyr-app-commands::
   :tool: cmake
   :app: samples/hello_world
   :generator: make
   :host-os: unix
   :board: reel_board
   :goals: build


Basics
======

#. Navigate to the application directory :file:`<app>`.
#. Enter the following commands to build the application's :file:`zephyr.elf`
   image for the board specified in the command-line parameters:

   .. zephyr-app-commands::
      :tool: all
      :cd-into:
      :board: <board>
      :goals: build

   If desired, you can build the application using the configuration settings
   specified in an alternate :file:`.conf` file using the :code:`CONF_FILE`
   parameter. These settings will override the settings in the application's
   :file:`.config` file or its default :file:`.conf` file. For example:

   .. zephyr-app-commands::
      :tool: all
      :cd-into:
      :board: <board>
      :gen-args: -DCONF_FILE=prj.alternate.conf
      :goals: build
      :compact:

   As described in the previous section, you can instead choose to permanently
   set the board and configuration settings by either exporting :makevar:`BOARD`
   and :makevar:`CONF_FILE` environment variables or by setting their values
   in your :file:`CMakeLists.txt` using ``set()`` statements.
   Additionally, ``west`` allows you to :ref:`set a default board
   <west-building-config>`.

.. _build-directory-contents:

Build Directory Contents
========================

When using the Ninja generator a build directory looks like this:

.. code-block:: none

   <app>/build
   ├── build.ninja
   ├── CMakeCache.txt
   ├── CMakeFiles
   ├── cmake_install.cmake
   ├── rules.ninja
   └── zephyr

The most notable files in the build directory are:

* :file:`build.ninja`, which can be invoked to build the application.

* A :file:`zephyr` directory, which is the working directory of the
  generated build system, and where most generated files are created and
  stored.

After running ``ninja``, the following build output files will be written to
the :file:`zephyr` sub-directory of the build directory. (This is **not the
Zephyr base directory**, which contains the Zephyr source code etc. and is
described above.)

* :file:`.config`, which contains the configuration settings
  used to build the application.

  .. note::

     The previous version of :file:`.config` is saved to :file:`.config.old`
     whenever the configuration is updated. This is for convenience, as
     comparing the old and new versions can be handy.

* Various object files (:file:`.o` files and :file:`.a` files) containing
  compiled kernel and application code.

* :file:`zephyr.elf`, which contains the final combined application and
  kernel binary. Other binary output formats, such as :file:`.hex` and
  :file:`.bin`, are also supported.

.. _application_rebuild:

Rebuilding an Application
=========================

Application development is usually fastest when changes are continually tested.
Frequently rebuilding your application makes debugging less painful
as the application becomes more complex. It's usually a good idea to
rebuild and test after any major changes to the application's source files,
CMakeLists.txt files, or configuration settings.

.. important::

    The Zephyr build system rebuilds only the parts of the application image
    potentially affected by the changes. Consequently, rebuilding an application
    is often significantly faster than building it the first time.

Sometimes the build system doesn't rebuild the application correctly
because it fails to recompile one or more necessary files. You can force
the build system to rebuild the entire application from scratch with the
following procedure:

#. Open a terminal console on your host computer, and navigate to the
   build directory :file:`<app>/build`.

#. Enter one of the following commands, depending on whether you want to use
   ``west`` or ``cmake`` directly to delete the application's generated
   files, except for the :file:`.config` file that contains the
   application's current configuration information.

   .. code-block:: console

       west build -t clean

   or

   .. code-block:: console

       ninja clean

   Alternatively, enter one of the following commands to delete *all*
   generated files, including the :file:`.config` files that contain
   the application's current configuration information for those board
   types.

   .. code-block:: console

       west build -t pristine

   or

   .. code-block:: console

       ninja pristine

   If you use west, you can take advantage of its capability to automatically
   :ref:`make the build folder pristine <west-building-config>` whenever it is
   required.

#. Rebuild the application normally following the steps specified
   in :ref:`build_an_application` above.

.. _application_board_version:

Building for a board revision
=============================

The Zephyr build system has support for specifying multiple hardware revisions
of a single board with small variations. Using revisions allows the board
support files to make minor adjustments to a board configuration without
duplicating all the files described in :ref:`create-your-board-directory` for
each revision.

To build for a particular revision, use ``<board>@<revision>`` instead of plain
``<board>``. For example:

.. zephyr-app-commands::
   :tool: all
   :cd-into:
   :board: <board>@<revision>
   :goals: build
   :compact:

Check your board's documentation for details on whether it has multiple
revisions, and what revisions are supported.

When targeting a board revision, the active revision will be printed at CMake
configure time, like this:

.. code-block:: console

   -- Board: plank, Revision: 1.5.0

.. _application_run:

Run an Application
******************

An application image can be run on a real board or emulated hardware.

.. _application_run_board:

Running on a Board
==================

Most boards supported by Zephyr let you flash a compiled binary using
the ``flash`` target to copy the binary to the board and run it.
Follow these instructions to flash and run an application on real
hardware:

#. Build your application, as described in :ref:`build_an_application`.

#. Make sure your board is attached to your host computer. Usually, you'll do
   this via USB.

#. Run one of these console commands from the build directory,
   :file:`<app>/build`, to flash the compiled Zephyr image and run it on
   your board:

   .. code-block:: console

      west flash

   or

   .. code-block:: console

      ninja flash

The Zephyr build system integrates with the board support files to
use hardware-specific tools to flash the Zephyr binary to your
hardware, then run it.

Each time you run the flash command, your application is rebuilt and flashed
again.

In cases where board support is incomplete, flashing via the Zephyr build
system may not be supported. If you receive an error message about flash
support being unavailable, consult :ref:`your board's documentation <boards>`
for additional information on how to flash your board.

.. note:: When developing on Linux, it's common to need to install
          board-specific udev rules to enable USB device access to
          your board as a non-root user. If flashing fails,
          consult your board's documentation to see if this is
          necessary.

.. _application_run_qemu:

Running in an Emulator
======================

The kernel has built-in emulator support for QEMU (on Linux/macOS only, this
is not yet supported on Windows). It allows you to run and test an application
virtually, before (or in lieu of) loading and running it on actual target
hardware. Follow these instructions to run an application via QEMU:

#. Build your application for one of the QEMU boards, as described in
   :ref:`build_an_application`.

   For example, you could set ``BOARD`` to:

   - ``qemu_x86`` to emulate running on an x86-based board
   - ``qemu_cortex_m3`` to emulate running on an ARM Cortex M3-based board

#. Run one of these console commands from the build directory,
   :file:`<app>/build`, to run the Zephyr binary in QEMU:

   .. code-block:: console

      west build -t run

   or

   .. code-block:: console

      ninja run

#. Press :kbd:`Ctrl A, X` to stop the application from running
   in QEMU.

   The application stops running and the terminal console prompt
   redisplays.

Each time you execute the run command, your application is rebuilt and run
again.


.. note::

   If the (Linux only) :ref:`Zephyr SDK <toolchain_zephyr_sdk>` is installed, the ``run``
   target will use the SDK's QEMU binary by default. To use another version of
   QEMU, :ref:`set the environment variable <env_vars>` ``QEMU_BIN_PATH``
   to the path of the QEMU binary you want to use instead.

.. note::

   You can choose a specific emulator by appending ``_<emulator>`` to your
   target name, for example ``west build -t run_qemu`` or ``ninja run_qemu``
   for QEMU.

.. _application_debugging:

Application Debugging
*********************

This section is a quick hands-on reference to start debugging your
application with QEMU. Most content in this section is already covered in
`QEMU`_ and `GNU_Debugger`_ reference manuals.

.. _QEMU: http://wiki.qemu.org/Main_Page

.. _GNU_Debugger: http://www.gnu.org/software/gdb

In this quick reference, you'll find shortcuts, specific environmental
variables, and parameters that can help you to quickly set up your debugging
environment.

The simplest way to debug an application running in QEMU is using the GNU
Debugger and setting a local GDB server in your development system through QEMU.

You will need an Executable and Linkable Format (ELF) binary image for
debugging purposes.  The build system generates the image in the build
directory.  By default, the kernel binary name is
:file:`zephyr.elf`. The name can be changed using a Kconfig option.

We will use the standard 1234 TCP port to open a :abbr:`GDB (GNU Debugger)`
server instance. This port number can be changed for a port that best suits the
development environment.

You can run QEMU to listen for a "gdb connection" before it starts executing any
code to debug it.

.. code-block:: bash

   qemu -s -S <image>

will setup Qemu to listen on port 1234 and wait for a GDB connection to it.

The options used above have the following meaning:

* ``-S`` Do not start CPU at startup; rather, you must type 'c' in the
  monitor.
* ``-s`` Shorthand for :literal:`-gdb tcp::1234`: open a GDB server on
  TCP port 1234.

To debug with QEMU and to start a GDB server and wait for a remote connect, run
either of the following inside the build directory of an application:

.. code-block:: bash

   ninja debugserver

The build system will start a QEMU instance with the CPU halted at startup
and with a GDB server instance listening at the TCP port 1234.

Using a local GDB configuration :file:`.gdbinit` can help initialize your GDB
instance on every run.
In this example, the initialization file points to the GDB server instance.
It configures a connection to a remote target at the local host on the TCP
port 1234. The initialization sets the kernel's root directory as a
reference.

The :file:`.gdbinit` file contains the following lines:

.. code-block:: bash

   target remote localhost:1234
   dir ZEPHYR_BASE

.. note::

   Substitute the correct :ref:`ZEPHYR_BASE <important-build-vars>` for your
   system.

Execute the application to debug from the same directory that you chose for
the :file:`gdbinit` file. The command can include the ``--tui`` option
to enable the use of a terminal user interface. The following commands
connects to the GDB server using :file:`gdb`. The command loads the symbol
table from the elf binary file. In this example, the elf binary file name
corresponds to :file:`zephyr.elf` file:

.. code-block:: bash

   ..../path/to/gdb --tui zephyr.elf

.. note::

   The GDB version on the development system might not support the --tui
   option. Please make sure you use the GDB binary from the SDK which
   corresponds to the toolchain that has been used to build the binary.

If you are not using a .gdbinit file, issue the following command inside GDB to
connect to the remote GDB server on port 1234:

.. code-block:: bash

   (gdb) target remote localhost:1234

Finally, the command below connects to the GDB server using the Data
Displayer Debugger (:file:`ddd`). The command loads the symbol table from the
elf binary file, in this instance, the :file:`zephyr.elf` file.

The :abbr:`DDD (Data Displayer Debugger)` may not be installed in your
development system by default. Follow your system instructions to install
it. For example, use ``sudo apt-get install ddd`` on an Ubuntu system.

.. code-block:: bash

   ddd --gdb --debugger "gdb zephyr.elf"


Both commands execute the :abbr:`gdb (GNU Debugger)`. The command name might
change depending on the toolchain you are using and your cross-development
tools.

.. _custom_board_definition:

Custom Board, Devicetree and SOC Definitions
********************************************

In cases where the board or platform you are developing for is not yet
supported by Zephyr, you can add board, Devicetree and SOC definitions
to your application without having to add them to the Zephyr tree.

The structure needed to support out-of-tree board and SOC development
is similar to how boards and SOCs are maintained in the Zephyr tree. By using
this structure, it will be much easier to upstream your platform related work into
the Zephyr tree after your initial development is done.

Add the custom board to your application or a dedicated repository using the
following structure:

.. code-block:: console

   boards/
   soc/
   CMakeLists.txt
   prj.conf
   README.rst
   src/

where the ``boards`` directory hosts the board you are building for:

.. code-block:: console

   .
   ├── boards
   │   └── x86
   │       └── my_custom_board
   │           ├── doc
   │           │   └── img
   │           └── support
   └── src

and the ``soc`` directory hosts any SOC code. You can also have boards that are
supported by a SOC that is available in the Zephyr tree.

Boards
======

Use the proper architecture folder name (e.g., ``x86``, ``arm``, etc.)
under ``boards`` for ``my_custom_board``.  (See  :ref:`boards` for a
list of board architectures.)

Documentation (under ``doc/``) and support files (under ``support/``) are optional, but
will be needed when submitting to Zephyr.

The contents of ``my_custom_board`` should follow the same guidelines for any
Zephyr board, and provide the following files::

    my_custom_board_defconfig
    my_custom_board.dts
    my_custom_board.yaml
    board.cmake
    board.h
    CMakeLists.txt
    doc/
    Kconfig.board
    Kconfig.defconfig
    pinmux.c
    support/


Once the board structure is in place, you can build your application
targeting this board by specifying the location of your custom board
information with the ``-DBOARD_ROOT`` parameter to the CMake
build system:

.. zephyr-app-commands::
   :tool: all
   :board: <board name>
   :gen-args: -DBOARD_ROOT=<path to boards>
   :goals: build
   :compact:

This will use your custom board configuration and will generate the
Zephyr binary into your application directory.

You can also define the ``BOARD_ROOT`` variable in the application
:file:`CMakeLists.txt` file. Make sure to do so **before** pulling in the Zephyr
boilerplate with ``find_package(Zephyr ...)``.

.. note::

   When specifying ``BOARD_ROOT`` in a CMakeLists.txt, then an absolute path must
   be provided, for example ``list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/<extra-board-root>)``.
   When using ``-DBOARD_ROOT=<board-root>`` both absolute and relative paths can
   be used. Relative paths are treated relatively to the application directory.

SOC Definitions
===============

Similar to board support, the structure is similar to how SOCs are maintained in
the Zephyr tree, for example:

.. code-block:: none

        soc
        └── arm
            └── st_stm32
                    ├── common
                    └── stm32l0



The file :zephyr_file:`soc/Kconfig` will create the top-level
``SoC/CPU/Configuration Selection`` menu in Kconfig.

Out of tree SoC definitions can be added to this menu using the ``SOC_ROOT``
CMake variable. This variable contains a semicolon-separated list of directories
which contain SoC support files.

Following the structure above, the following files can be added to load
more SoCs into the menu.

.. code-block:: none

        soc
        └── arm
            └── st_stm32
                    ├── Kconfig
                    ├── Kconfig.soc
                    └── Kconfig.defconfig

The Kconfig files above may describe the SoC or load additional SoC Kconfig files.

An example of loading ``stm31l0`` specific Kconfig files in this structure:

.. code-block:: none

        soc
        └── arm
            └── st_stm32
                    ├── Kconfig.soc
                    └── stm32l0
                        └── Kconfig.series

can be done with the following content in ``st_stm32/Kconfig.soc``:

.. code-block:: none

   rsource "*/Kconfig.series"

Once the SOC structure is in place, you can build your application
targeting this platform by specifying the location of your custom platform
information with the ``-DSOC_ROOT`` parameter to the CMake
build system:

.. zephyr-app-commands::
   :tool: all
   :board: <board name>
   :gen-args: -DSOC_ROOT=<path to soc> -DBOARD_ROOT=<path to boards>
   :goals: build
   :compact:

This will use your custom platform configurations and will generate the
Zephyr binary into your application directory.

See :ref:`modules_build_settings` for information on setting SOC_ROOT in a module's
:file:`zephyr/module.yml` file.

Or you can define the ``SOC_ROOT`` variable in the application
:file:`CMakeLists.txt` file. Make sure to do so **before** pulling in the
Zephyr boilerplate with ``find_package(Zephyr ...)``.

.. note::

   When specifying ``SOC_ROOT`` in a CMakeLists.txt, then an absolute path must
   be provided, for example ``list(APPEND SOC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/<extra-soc-root>``.
   When using ``-DSOC_ROOT=<soc-root>`` both absolute and relative paths can be
   used. Relative paths are treated relatively to the application directory.

.. _dts_root:

Devicetree Definitions
======================

Devicetree directory trees are found in ``APPLICATION_SOURCE_DIR``,
``BOARD_DIR``, and ``ZEPHYR_BASE``, but additional trees, or DTS_ROOTs,
can be added by creating this directory tree::

    include/
    dts/common/
    dts/arm/
    dts/
    dts/bindings/

Where 'arm' is changed to the appropriate architecture. Each directory
is optional. The binding directory contains bindings and the other
directories contain files that can be included from DT sources.

Once the directory structure is in place, you can use it by specifying
its location through the ``DTS_ROOT`` CMake Cache variable:

.. zephyr-app-commands::
   :tool: all
   :board: <board name>
   :gen-args: -DDTS_ROOT=<path to dts root>
   :goals: build
   :compact:

You can also define the variable in the application :file:`CMakeLists.txt`
file. Make sure to do so **before** pulling in the Zephyr boilerplate with
``find_package(Zephyr ...)``.

.. note::

   When specifying ``DTS_ROOT`` in a CMakeLists.txt, then an absolute path must
   be provided, for example ``list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/<extra-dts-root>``.
   When using ``-DDTS_ROOT=<dts-root>`` both absolute and relative paths can be
   used. Relative paths are treated relatively to the application directory.

Devicetree source are passed through the C preprocessor, so you can
include files that can be located in a ``DTS_ROOT`` directory.  By
convention devicetree include files have a ``.dtsi`` extension.

You can also use the preprocessor to control the content of a devicetree
file, by specifying directives through the ``DTS_EXTRA_CPPFLAGS`` CMake
Cache variable:

.. zephyr-app-commands::
   :tool: all
   :board: <board name>
   :gen-args: -DDTS_EXTRA_CPPFLAGS=-DTEST_ENABLE_FEATURE
   :goals: build
   :compact:



Debug with Eclipse
******************

Overview
========

CMake supports generating a project description file that can be imported into
the Eclipse Integrated Development Environment (IDE) and used for graphical
debugging.

The `GNU MCU Eclipse plug-ins`_ provide a mechanism to debug ARM projects in
Eclipse with pyOCD, Segger J-Link, and OpenOCD debugging tools.

The following tutorial demonstrates how to debug a Zephyr application in
Eclipse with pyOCD in Windows. It assumes you have already installed the GCC
ARM Embedded toolchain and pyOCD.

Set Up the Eclipse Development Environment
==========================================

#. Download and install `Eclipse IDE for C/C++ Developers`_.

#. In Eclipse, install the GNU MCU Eclipse plug-ins by opening the menu
   ``Window->Eclipse Marketplace...``, searching for ``GNU MCU Eclipse``, and
   clicking ``Install`` on the matching result.

#. Configure the path to the pyOCD GDB server by opening the menu
   ``Window->Preferences``, navigating to ``MCU``, and setting the ``Global
   pyOCD Path``.

Generate and Import an Eclipse Project
======================================

#. Set up a GNU Arm Embedded toolchain as described in
   :ref:`toolchain_gnuarmemb`.

#. Navigate to a folder outside of the Zephyr tree to build your application.

   .. code-block:: console

      # On Windows
      cd %userprofile%

   .. note::
      If the build directory is a subdirectory of the source directory, as is
      usually done in Zephyr, CMake will warn:

      "The build directory is a subdirectory of the source directory.

      This is not supported well by Eclipse.  It is strongly recommended to use
      a build directory which is a sibling of the source directory."

#. Configure your application with CMake and build it with ninja. Note the
   different CMake generator specified by the ``-G"Eclipse CDT4 - Ninja"``
   argument. This will generate an Eclipse project description file,
   :file:`.project`, in addition to the usual ninja build files.

   .. zephyr-app-commands::
      :tool: all
      :app: %ZEPHYR_BASE%\samples\synchronization
      :host-os: win
      :board: frdm_k64f
      :gen-args: -G"Eclipse CDT4 - Ninja"
      :goals: build
      :compact:

#. In Eclipse, import your generated project by opening the menu
   ``File->Import...`` and selecting the option ``Existing Projects into
   Workspace``. Browse to your application build directory in the choice,
   ``Select root directory:``. Check the box for your project in the list of
   projects found and click the ``Finish`` button.

Create a Debugger Configuration
===============================

#. Open the menu ``Run->Debug Configurations...``.

#. Select ``GDB PyOCD Debugging``, click the ``New`` button, and configure the
   following options:

   - In the Main tab:

     - Project: ``my_zephyr_app@build``
     - C/C++ Application: :file:`zephyr/zephyr.elf`

   - In the Debugger tab:

     - pyOCD Setup

       - Executable path: :file:`${pyocd_path}\\${pyocd_executable}`
       - Uncheck "Allocate console for semihosting"

     - Board Setup

       - Bus speed: 8000000 Hz
       - Uncheck "Enable semihosting"

     - GDB Client Setup

       - Executable path example (use your ``GNUARMEMB_TOOLCHAIN_PATH``):
         :file:`C:\\gcc-arm-none-eabi-6_2017-q2-update\\bin\\arm-none-eabi-gdb.exe`

   - In the SVD Path tab:

     - File path: :file:`<workspace
       top>\\modules\\hal\\nxp\\mcux\\devices\\MK64F12\\MK64F12.xml`

     .. note::
	This is optional. It provides the SoC's memory-mapped register
	addresses and bitfields to the debugger.

#. Click the ``Debug`` button to start debugging.

RTOS Awareness
==============

Support for Zephyr RTOS awareness is implemented in `pyOCD v0.11.0`_ and later.
It is compatible with GDB PyOCD Debugging in Eclipse, but you must enable
CONFIG_DEBUG_THREAD_INFO=y in your application.



.. _CMake: https://www.cmake.org
.. _CMake introduction: https://cmake.org/cmake/help/latest/manual/cmake.1.html#description
.. _Eclipse IDE for C/C++ Developers: https://www.eclipse.org/downloads/packages/eclipse-ide-cc-developers/oxygen2
.. _GNU MCU Eclipse plug-ins: https://gnu-mcu-eclipse.github.io/plugins/install/
.. _pyOCD v0.11.0: https://github.com/mbedmicro/pyOCD/releases/tag/v0.11.0
.. _CMake list: https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#lists
.. _add_subdirectory(): https://cmake.org/cmake/help/latest/command/add_subdirectory.html
.. _using Chocolatey: https://chocolatey.org/packages/RapidEE
.. _example-application: https://github.com/zephyrproject-rtos/example-application
