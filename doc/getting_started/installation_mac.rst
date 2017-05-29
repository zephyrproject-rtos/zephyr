.. _installing_zephyr_mac:

Development Environment Setup on Mac OS
#######################################

This section describes how to set up a Mac OS development system.

After completing these steps, you will be able to compile and run your Zephyr
applications on the following Mac OS version:

* Mac OS X 10.11 (El Capitan)
* macOS Sierra 10.12

Developing for Zephyr on macOS generally requires you to build the
toolchain yourself. However, if there is already an macOS toolchain for your
target architecture you can use it directly.

Using a 3rd Party toolchain
***************************

If a toolchain is available for the architecture you plan to build for, then
you can use it as explained in: :ref:`third_party_x_compilers`.

An example of an available 3rd party toolchain is GCC ARM Embedded for the
Cortex-M family of cores.

.. _mac_requirements:

Installing Requirements and Dependencies
****************************************

To install the software components required to build the Zephyr kernel on a
Mac, you will need to build a cross compiler for the target devices you wish to
build for and install tools that the build system requires.

.. note::
   Minor version updates of the listed required packages might also
   work.

Before proceeding with the build, ensure your OS is up to date.

First, install the :program:`Homebrew` (The missing package manager for
macOS). Homebrew is a free and open-source software package management system
that simplifies the installation of software on Apple's macOS operating
system.

To install :program:`Homebrew`, visit the `Homebrew site`_ and follow the
installation instructions on the site.

To complete the Homebrew installation, you might be prompted to install some
missing dependency. If so, follow please follow the instructions provided.

After Homebrew was successfully installed, install the following tools using
the brew command line.

Install tools to build Zephyr binaries:

.. code-block:: console

   $ brew install dfu-util qemu dtc python3
   $ pip3 install ply pyyaml

Install tools needed for building the toolchain (if needed):

.. code-block:: console

   $ brew install gettext help2man mpfr gmp coreutils wget
   $ brew tap homebrew/dupes
   $ brew install grep --with-default-names


To build the toolchain, you will need the latest version of crosstool-ng (1.23).
This version was not available via brew when writing this documentation, you can
however try and see if you get 1.23 installed:

.. code-block:: console

   $ brew install crosstool-ng

Alternatively you can install the latest version of :program:`crosstool-ng`
from source. Download the latest version from the `crosstool-ng site`_. The
latest version usually supports the latest released compilers.

.. code-block:: console

   $ wget http://crosstool-ng.org/download/crosstool-ng/crosstool-ng-1.23.0.tar.bz2
   $ tar xvf crosstool-ng-1.23.0.tar.bz2
   $ cd crosstool-ng-1.23.0/
   $ ./configure
   $ make
   $ make install

.. _setting_up_mac_toolchain:

Setting Up the Toolchain
************************

Creating a Case-sensitive File System
=====================================

Building the compiler requires a case-sensitive file system. Therefore, use
:program:`diskutil` to create an 8 GB blank sparse image making sure you select
case-sensitive file system (OS X Extended (Case-sensitive, Journaled) and
mount it.

Alternatively you can use the script below to create the image:

.. code-block:: bash

   #!/bin/bash
   ImageName=CrossToolNG
   ImageNameExt=${ImageName}.sparseimage
   diskutil umount force /Volumes/${ImageName} && true
   rm -f ${ImageNameExt} && true
   hdiutil create ${ImageName} -volname ${ImageName} -type SPARSE -size 8g -fs HFSX
   hdiutil mount ${ImageNameExt}
   cd /Volumes/$ImageName

When mounted, the file system of the image will be available under
:file:`/Volumes`. Change to the mounted directory:

.. code-block:: console

   $ cd /Volumes/CrossToolNG
   $ mkdir build
   $ cd build

Setting the Toolchain Options
=============================

In the Zephyr kernel source tree we provide two configurations for
both ARM and X86 that can be used to pre-select the options needed
for building the toolchain.
The configuration files can be found in :file:`${ZEPHYR_BASE}/scripts/cross_compiler/`.

Currently the following configurations are provided:

* i586.config: for standard ABI, for example for Galileo and qemu_x86
* iamcu.config: for IAMCU ABI, for example for the Arduino 101
* nios2.config: for Nios II boards

.. code-block:: console

   $ cp ${ZEPHYR_BASE}/scripts/cross_compiler/i586.config .config

You can create a toolchain configuration or customize an existing configuration
yourself using the configuration menus:

.. code-block:: console

   $ export CT_PREFIX=/Volumes/CrossToolNG
   $ ct-ng menuconfig

Verifying the Configuration of the Toolchain
============================================

Before building the toolchain it is advisable to perform a quick verification
of the configuration set for the toolchain.

1. Open the generated :file:`.config` file.

2. Verify the following lines are present, assuming the sparse image was
   mounted under :file:`/Volumes/CrossToolNG`:

.. code-block:: bash

   ...
   CT_LOCAL_TARBALLS_DIR="/Volumes/CrossToolNG/src"
   # CT_SAVE_TARBALLS is not set
   CT_WORK_DIR="${CT_TOP_DIR}/.build"
   CT_PREFIX_DIR="/Volumes/CrossToolNG/x-tools/${CT_TARGET}"
   CT_INSTALL_DIR="${CT_PREFIX_DIR}"
   # Following options prevent link errors
   CT_WANTS_STATIC_LINK=n
   CT_CC_STATIC_LIBSTDCXX=n
   ...

Building the Toolchain
======================

To build the toolchain, enter:

.. code-block:: console

   $ ct-ng build

The above process takes a while. When finished, the toolchain will be available
under :file:`/Volumes/CrossToolNG/x-tools`.

Repeat the step for all architectures you want to support in your environment.

To use the toolchain with Zephyr, export the following environment variables
and use the target location where the toolchain was installed, type:

.. code-block:: console

   $ export ZEPHYR_GCC_VARIANT=xtools
   $ export XTOOLS_TOOLCHAIN_PATH=/Volumes/CrossToolNG/x-tools


To use the same toolchain in new sessions in the future you can set the
variables in the file :file:`${HOME}/.zephyrrc`, for example:

.. code-block:: console

   $ cat <<EOF > ~/.zephyrrc
   export XTOOLS_TOOLCHAIN_PATH=/Volumes/CrossToolNG/x-tools
   export ZEPHYR_GCC_VARIANT=xtools
   EOF

.. _Homebrew site: http://brew.sh/

.. _crosstool-ng site: http://crosstool-ng.org
