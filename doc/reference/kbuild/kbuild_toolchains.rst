.. _kbuild_toolchains:

Using Toolchains with Kbuild
****************************

The |project| gives support for the configuration of Yocto and XTools
toolchain and build tools. The environment variable
:envvar:`ZEPHYR_GCC_VARIANT` informs the build systen about which
build tool set is installed in the system and configures it as a standard
installation:

.. code-block:: bash

   $ export ZEPHYR_GCC_VARIANT = yocto

   $ export ZEPHYR_GCC_VARIANT = xtools

The supported values for the :envvar:`ZEPHYR_GCC_VARIANT` variable are:
**yocto** and **xtools**.

Yocto Configuration
===================

To set up a previously installed Yocto toolchain in the build system,
you need to configure the Yocto SDK installation path and the GCC
variant in the shell environment:

.. code-block:: bash

   $ export YOCTO_SDK_INSTALL_DIR = <yocto-installation-path>

   $ export ZEPHYR_GCC_VARIANT = yocto

The build system configuration is done by the file
:file:`$(ZEPHYR_BASE)/scripts/Makefile.toochain.yocto`. The build
system takes the following configuration values:

* x86 default configuration values

  * Crosscompile target: i586-poky-elf

  * Crosscompile version: 4.9.3

  * Toolchain library: gcc


* ARM default configuration values

  * Crosscompile target: arm-poky-eabi

  * Crosscompile version: 5.2.0

  * Toolchain library: gcc

* ARC default configuration values

  * Crosscompile target: arc-poky-elf

  * Crosscompile version: 4.8.3

  * Toolchain library: gcc

The cross-compile target, cross-compile version, toolchain library and
library path can be adjusted in the file
:file:`$(ZEPHYR_BASE)/scripts/Makefile.toochain.yocto` following your
installation specifics.

XTools Configuration
====================

To set up a previously installed XTools toolchain in the build system,
you need to configure the XTools installation path and the GCC
variant in the shell environment:

.. code-block:: bash

   $ export XTOOLS_TOOLCHAIN_PATH = <yocto-installation-path>

   $ export ZEPHYR_GCC_VARIANT = xtools

The build system configuration is done by the file
:file:`$(ZEPHYR_BASE)/scripts/Makefile.toochain.xtools`. The build
system takes the following configuration values:

* x86 default configuration values

  * Crosscompile target: i586-pc-elf

  * Crosscompile version: 4.9.2

  * Toolchain library: gcc

* ARM default configuration values

  * Crosscompile target: arm-none-eabi

  * Crosscompile version: 4.9.2

  * Toolchain library: gcc

The cross-compile target, cross-compile version and toolchain
can be adjusted in the file
:file:`$(ZEPHYR_BASE)/scripts/Makefile.toochain.xtools` following your
installation specifics.

Generic Toolchain Configuration
===============================

It is possible to build and install an specific toolchain and configure
the build system to work with it. The **CROSS_COMPILE**,
**TOOLCHAIN_LIBS** and **LIB_INCLUDE_DIR** need to be configured in
your environment.

.. note::

   The installed toolchain must be from the gcc family. The build tools
   should follow the convention of: prefix + command-name. For example,
   the gcc command should be named: **arm-poky-eabi-gcc**

The **CROSS_COMPILE** environment variable should be set to the
build tools prefix used for build tools commands.

.. code-block:: bash

   $ export CROSS_COMPILE = i586-elf-

.. note::
   If the command home directory is not set in the **PATH** environment
   variable, the **CROSS_COMPILE** must include the complete path as
   part of the command prefix.

The **TOOLCHAIN_LIBS** list the libraries required by the toolchain, like gcc
.

.. code-block:: bash

   $ export TOOLCHAIN_LIBS = gcc

.. note::
   Notice that there  library name does not include the l prefix
   commonly found when referring to libraries (lgcc).

**LIB_INCLUDE_DIR** defines the directory path where the toolchain
libraries can be located.

.. code-block:: bash

   $ export LIB_INCLUDE_DIR = -L /opt/i586-elf/usr/lib/i586-elf/4.9

.. note::
   Notice the use of the -L command parameter, included in the value
   of the environment variable.
