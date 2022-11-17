.. _sysbuild:

Sysbuild (System build)
#######################

Sysbuild is a higher-level build system that can be used to combine multiple
other build systems together. It is a higher-level layer that combines one
or more Zephyr build systems and optional additional build systems
into a hierarchical build system.

For example, you can use sysbuild to build a Zephyr application together
with the MCUboot bootloader, flash them both onto your device, and
debug the results.

Sysbuild works by configuring and building at least a Zephyr application and, optionally, as many
additional projects as you want. The additional projects can be either Zephyr applications
or other types of builds you want to run.

Like Zephyr's :ref:`build system <build_overview>`, sysbuild is written in
CMake and uses :ref:`Kconfig <kconfig>`.

Definitions
***********

The following are some key concepts used in this document:

Single-image build
    When sysbuild is used to create and manage just one Zephyr application's
    build system.

Multi-image build
   When sysbuild is used to manage multiple build systems.
   The word "image" is used because your main goal is usually to generate the binaries of the firmware
   application images from each build system.

Domain
   Every Zephyr CMake build system managed by sysbuild.

Multi-domain
   When more than one Zephyr CMake build system (domain) is managed by sysbuild.

Architectural Overview
**********************

This figure is an overview of sysbuild's inputs, outputs, and user interfaces:

.. figure:: sysbuild.svg
    :align: center
    :alt: Sysbuild architectural overview
    :figclass: align-center
    :width: 80%

The following are some key sysbuild features indicated in this figure:

- You can run sysbuild either with :ref:`west build
  <west-building>` or directly via ``cmake``.

- You can use sysbuild to generate application images from each build system,
  shown above as ELF, BIN, and HEX files.

- You can configure sysbuild or any of the build systems it manages using
  various configuration variables. These variables are namespaced so that
  sysbuild can direct them to the right build system. In some cases, such as
  the ``BOARD`` variable, these are shared among multiple build systems.

- Sysbuild itself is also configured using Kconfig. For example, you can
  instruct sysbuild to build the MCUboot bootloader, as well as to build and
  link your main Zephyr application as an MCUboot child image, using sysbuild's
  Kconfig files.

- Sysbuild integrates with west's :ref:`west-build-flash-debug` commands. It
  does this by managing the :ref:`west-runner`, and specifically the
  :file:`runners.yaml` files that each Zephyr build system will contain. These
  are packaged into a global view of how to flash and debug each build system
  in a :file:`domains.yaml` file generated and managed by sysbuild.

Building with sysbuild
**********************

As mentioned above, you can run sysbuild via ``west build`` or ``cmake``.

.. tabs::

   .. group-tab:: ``west build``

      Here is an example. For details, see :ref:`west-multi-domain-builds` in
      the ``west build documentation``.

      .. zephyr-app-commands::
         :tool: west
         :app: samples/hello_world
         :board: reel_board
         :goals: build
         :west-args: --sysbuild
         :compact:

      .. tip::

         To configure ``west build`` to use ``--sysbuild`` by default from now on,
         run::

           west config build.sysbuild True

         Since sysbuild supports both single- and multi-image builds, this lets you
         use sysbuild all the time, without worrying about what type of build you are
         running.

         To turn this off, run this before generating your build system::

           west config build.sysbuild False

         To turn this off for just one ``west build`` command, run::

           west build --no-sysbuild ...

   .. group-tab:: ``cmake``

      Here is an example using CMake and Ninja.

      .. zephyr-app-commands::
         :tool: cmake
         :app: share/sysbuild
         :board: reel_board
         :goals: build
         :gen-args: -DAPP_DIR=samples/hello_world
         :compact:

      To use sysbuild directly with CMake, you must specify the sysbuild
      project as the source folder, and give ``-DAPP_DIR=<path-to-sample>`` as
      an extra CMake argument. ``APP_DIR`` is the path to the main Zephyr
      application managed by sysbuild.

Configuration namespacing
*************************

When building a single Zephyr application without sysbuild, all CMake cache
settings and Kconfig build options given on the command line as
``-D<var>=<value>`` or ``-DCONFIG_<var>=<value>`` are handled by the Zephyr
build system.

However, when sysbuild combines multiple Zephyr build systems, there could be
Kconfig settings exclusive to sysbuild (and not used by any of the applications).
To handle this, sysbuild has namespaces for configuration variables. You can use these
namespaces to direct settings either to sysbuild itself or to a specific Zephyr
application managed by sysbuild using the information in these sections.

The following example shows how to build :ref:`hello_world` with MCUboot enabled,
applying to both images debug optimizations:

.. tabs::

   .. group-tab:: ``west build``

      .. zephyr-app-commands::
         :tool: west
         :app: samples/hello_world
         :board: reel_board
         :goals: build
         :west-args: --sysbuild
         :gen-args: -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_DEBUG_OPTIMIZATIONS=y -Dmcuboot_DEBUG_OPTIMIZATIONS=y
         :compact:

   .. group-tab:: ``cmake``

      .. zephyr-app-commands::
         :tool: cmake
         :app: share/sysbuild
         :board: reel_board
         :goals: build
         :gen-args: -DAPP_DIR=samples/hello_world -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_DEBUG_OPTIMIZATIONS=y -Dmcuboot_DEBUG_OPTIMIZATIONS=y
         :compact:

See the following subsections for more information.

.. _sysbuild_cmake_namespace:

CMake variable namespacing
==========================

CMake variable settings can be passed to CMake using ``-D<var>=<value>`` on the
command line. You can also set Kconfig options via CMake as
``-DCONFIG_<var>=<value>`` or ``-D<namespace>_CONFIG_<var>=<value>``.

Since sysbuild is the entry point for the build system, and sysbuild is written
in CMake, all CMake variables are first processed by sysbuild.

Sysbuild creates a namespace for each domain. The namespace prefix is the
domain's application name. See :ref:`sysbuild_zephyr_application` for more
information.

To set the variable ``<var>`` in the namespace ``<namespace>``, use this syntax::

  -D<namespace>_<var>=<value>

For example, to set the CMake variable ``FOO`` in the ``my_sample`` application
build system to the value ``BAR``, run the following commands:

.. tabs::

   .. group-tab:: ``west build``

      ::

         west build --sysbuild ... -- -Dmy_sample_FOO=BAR

   .. group-tab:: ``cmake``

      ::

        cmake -Dmy_sample_FOO=BAR ...

.. _sysbuild_kconfig_namespacing:

Kconfig namespacing
===================

To set the sysbuild Kconfig option ``<var>`` to the value ``<value>``, use this syntax::

  -DSB_CONFIG_<var>=<value>

In the previous example, ``SB_CONFIG`` is the namespace prefix for sysbuild's Kconfig
options.

To set a Zephyr application's Kconfig option instead, use this syntax::

  -D<namespace>_CONFIG_<var>=<value>

In the previous example, ``<namespace>`` is the application name discussed above in
:ref:`sysbuild_cmake_namespace`.

For example, to set the Kconfig option ``FOO`` in the ``my_sample`` application
build system to the value ``BAR``, run the following commands:

.. tabs::

   .. group-tab:: ``west build``

      ::

         west build --sysbuild ... -- -Dmy_sample_CONFIG_FOO=BAR

   .. group-tab:: ``cmake``

      ::

        cmake -Dmy_sample_CONFIG_FOO=BAR ...

.. tip::
   When no ``<namespace>`` is used, the Kconfig setting is passed to the main
   Zephyr application ``my_sample``.

   This means that passing ``-DCONFIG_<var>=<value>`` and
   ``-Dmy_sample_CONFIG_<var>=<value>`` are equivalent.

   This allows you to build the same application with or without sysbuild using
   the same syntax for setting Kconfig values at CMake time.
   For example, the following commands will work in the same way:

   ::

     west build -b <board> my_sample -- -DCONFIG_FOO=BAR

   ::

     west build -b <board> --sysbuild my_sample -- -DCONFIG_FOO=BAR

Sysbuild flashing using ``west flash``
**************************************

You can use :ref:`west flash <west-flashing>` to flash applications with
sysbuild.

When invoking ``west flash`` on a build consisting of multiple images, each
image is flashed in sequence. Extra arguments such as ``--runner jlink`` are
passed to each invocation.

For more details, see :ref:`west-multi-domain-flashing`.

Sysbuild debugging using ``west debug``
***************************************

You can use ``west debug``  to debug the main application, whether you are using sysbuild or not.
Just follow the existing :ref:`west debug <west-debugging>` guide to debug the main sample.

To debug a different domain (Zephyr application), such as ``mcuboot``, use
the ``--domain`` argument, as follows::

  west debug --domain mcuboot

For more details, see :ref:`west-multi-domain-debugging`.

Building a sample with MCUboot
******************************

Sysbuild supports MCUboot natively.

To build a sample like ``hello_world`` with MCUboot,
enable MCUboot and build and flash the sample as follows:

.. tabs::

   .. group-tab:: ``west build``

      .. zephyr-app-commands::
         :tool: west
         :app: samples/hello_world
         :board: reel_board
         :goals: build
         :west-args: --sysbuild
         :gen-args: -DSB_CONFIG_BOOTLOADER_MCUBOOT=y
         :compact:

   .. group-tab:: ``cmake``

      .. zephyr-app-commands::
         :tool: cmake
         :app: share/sysbuild
         :board: reel_board
         :goals: build
         :gen-args: -DAPP_DIR=samples/hello_world -DSB_CONFIG_BOOTLOADER_MCUBOOT=y
         :compact:

This builds ``hello_world`` and ``mcuboot`` for the ``reel_board``, and then
flashes both the ``mcuboot`` and ``hello_world`` application images to the
board.

More detailed information regarding the use of MCUboot with Zephyr can be found
in the `MCUboot with Zephyr`_ documentation page on the MCUboot website.

.. note::

   MCUBoot default configuration will perform a full chip erase when flashed.
   This can be controlled through the MCUBoot Kconfig option
   ``CONFIG_ZEPHYR_TRY_MASS_ERASE``. If this option is enabled, then flashing
   only MCUBoot, for example using ``west flash --domain mcuboot``, may erase
   the entire flash, including the main application image.

Sysbuild Kconfig file
*********************

You can set sysbuild's Kconfig options for a single application using
configuration files. By default, sysbuild looks for a configuration file named
``sysbuild.conf`` in the application top-level directory.

In the following example, there is a :file:`sysbuild.conf` file that enables building and flashing with
MCUboot whenever sysbuild is used:

.. code-block:: none

   <home>/application
   ├── CMakeLists.txt
   ├── prj.conf
   └── sysbuild.conf


.. code-block:: none

   SB_CONFIG_BOOTLOADER_MCUBOOT=y

You can set a configuration file to use with the
``-DSB_CONF_FILE=<sysbuild-conf-file>`` CMake build setting.

For example, you can create ``sysbuild-mcuboot.conf`` and then
specify this file when building with sysbuild, as follows:

.. tabs::

   .. group-tab:: ``west build``

      .. zephyr-app-commands::
         :tool: west
         :app: samples/hello_world
         :board: reel_board
         :goals: build
         :west-args: --sysbuild
         :gen-args: -DSB_CONF_FILE=sysbuild-mcuboot.conf
         :compact:

   .. group-tab:: ``cmake``

      .. zephyr-app-commands::
         :tool: cmake
         :app: share/sysbuild
         :board: reel_board
         :goals: build
         :gen-args: -DAPP_DIR=samples/hello_world -DSB_CONF_FILE=sysbuild-mcuboot.conf
         :compact:

.. _sysbuild_zephyr_application:

Adding Zephyr applications to sysbuild
**************************************

You can use the ``ExternalZephyrProject_Add()`` function to add Zephyr
applications as sysbuild domains. Call this CMake function from your
application's :file:`sysbuild.cmake` file, or any other CMake file you know will
run as part sysbuild CMake invocation.

Targeting the same board
========================

To include ``my_sample`` as another sysbuild domain, targeting the same board
as the main image, use this example:

.. code-block:: cmake

   ExternalZephyrProject_Add(
     APPLICATION my_sample
     SOURCE_DIR <path-to>/my_sample
   )

This could be useful, for example, if your board requires you to build and flash an
SoC-specific bootloader along with your main application.

Targeting a different board
===========================

In sysbuild and Zephyr CMake build system a board may refer to:

* A physical board with a single core SoC.
* A specific core on a physical board with a multi-core SoC, such as
  :ref:`nrf5340dk_nrf5340`.
* A specific SoC on a physical board with multiple SoCs, such as
  :ref:`nrf9160dk_nrf9160` and :ref:`nrf9160dk_nrf52840`.

If your main application, for example, is built for ``mps2_an521``, and your
helper application must target the ``mps2_an521_remote`` board (cpu1), add
a CMake function call that is structured as follows:

.. code-block:: cmake

   ExternalZephyrProject_Add(
     APPLICATION my_sample
     SOURCE_DIR <path-to>/my_sample
     BOARD mps2_an521_remote
   )

This could be useful, for example, if your main application requires another
helper Zephyr application to be built and flashed alongside it, but the helper
runs on another core in your SoC.

Targeting conditionally using Kconfig
=====================================

You can control whether extra applications are included as sysbuild domains
using Kconfig.

If the extra application image is specific to the board or an application,
you can create two additional files: :file:`sysbuild.cmake` and :file:`Kconfig.sysbuild`.

For an application, this would look like this:

.. code-block:: none

   <home>/application
   ├── CMakeLists.txt
   ├── prj.conf
   ├── Kconfig.sysbuild
   └── sysbuild.cmake

In the previous example, :file:`sysbuild.cmake` would be structured as follows:

.. code-block:: cmake

   if(SB_CONFIG_SECOND_SAMPLE)
     ExternalZephyrProject_Add(
       APPLICATION second_sample
       SOURCE_DIR <path-to>/second_sample
     )
   endif()

:file:`Kconfig.sysbuild` would be structured as follows:

.. code-block:: kconfig

   source "sysbuild/Kconfig"

   config SECOND_SAMPLE
           bool "Second sample"
           default y

This will include ``second_sample`` by default, while still allowing you to
disable it using the Kconfig option ``SECOND_SAMPLE``.

For more information on setting sysbuild Kconfig options,
see :ref:`sysbuild_kconfig_namespacing`.

Zephyr application configuration
================================

When adding a Zephyr application to sysbuild, such as MCUboot, then the
configuration files from the application (MCUboot) itself will be used.

When integrating multiple applications with each other, then it is often
necessary to make adjustments to the configuration of extra images.

Sysbuild gives users the ability of creating Kconfig fragments or devicetree
overlays that will be used together with the application's default configuration.
Sysbuild also allows users to change :ref:`application-configuration-directory`
in order to give users full control of an image's configuration.

Zephyr application Kconfig fragment and devicetree overlay
----------------------------------------------------------

In the folder of the main application, create a Kconfig fragment or a devicetree
overlay under a sysbuild folder, where the name of the file is
:file:`<image>.conf` or :file:`<image>.overlay`, for example if your main
application includes ``my_sample`` then create a :file:`sysbuild/my_sample.conf`
file or a devicetree overlay :file:`sysbuild/my_sample.overlay`.

A Kconfig fragment could look as:

.. code-block:: none

   # sysbuild/my_sample.conf
   CONFIG_FOO=n

Zephyr application configuration directory
------------------------------------------

In the folder of the main application, create a new folder under
:file:`sysbuild/<image>/`.
This folder will then be used as ``APPLICATION_CONFIG_DIR`` when building
``<image>``.
As an example, if your main application includes ``my_sample`` then create a
:file:`sysbuild/my_sample/` folder and place any configuration files in
there as you would normally do:

.. code-block:: none

   <home>/application
   ├── CMakeLists.txt
   ├── prj.conf
   └── sysbuild
       └── my_sample
           ├── prj.conf
           ├── app.overlay
           └── boards
               ├── <board_A>.conf
               ├── <board_A>.overlay
               ├── <board_B>.conf
               └── <board_B>.overlay

All configuration files under the :file:`sysbuild/my_sample/` folder will now
be used when ``my_sample`` is included in the build, and the default
configuration files for ``my_sample`` will be ignored.

This give you full control on how images are configured when integrating those
with ``application``.

Adding non-Zephyr applications to sysbuild
******************************************

You can include non-Zephyr applications in a multi-image build using the
standard CMake module `ExternalProject`_. Please refer to the CMake
documentation for usage details.

When using ``ExternalProject``, the non-Zephyr application will be built as
part of the sysbuild build invocation, but ``west flash`` or ``west debug``
will not be aware of the application. Instead, you must manually flash and
debug the application.

.. _MCUboot with Zephyr: https://mcuboot.com/documentation/readme-zephyr/
.. _ExternalProject: https://cmake.org/cmake/help/latest/module/ExternalProject.html
