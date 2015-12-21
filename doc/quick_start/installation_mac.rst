.. _installing_zephyr_mac:

Development Environment Setup on Mac OS
#######################################

This section describes how to build the kernel in a development system.

This guide was tested by compiling and running the Zephyr Kernel's sample
applications on the following Mac OS version:

* Mac OS X 10.11 (El Capitan)

Update Your Operating System
****************************

Before proceeding with the build, ensure your OS is up to date.

.. _mac_requirements:

Installing Requirements and Dependencies
****************************************

To install the software components required to build the Zephyr kernel on a
Mac, you will need to build a cross compiler for the target devices you wish to
build for and install tools that the build system requires.

First, install the :program:`Homebrew` (The missing package manager for
OS X). Homebrew is a free and open-source software package management system
that simplifies the installation of software on Apple's OS X operating
system.

To install :program:`Homebrew`, visit the site `<http://brew.sh/>`_ and follow the
installation instructions on the site.

To complete the Homebrew installation, you might be prompted to install some
missing dependency. If so, follow please follow the instructions provided.

After Homebrew was successfuly installed, install the following tools using the
brew command line.

.. code-block:: console

   $ brew install gettext qemu help2man mpfr gmp coreutils wget

   $ brew tap homebrew/dupes

   $ brew install grep --default-names

.. code-block:: console

   $ brew install crosstool-ng

Alternatively you can install the latest version of :program:`crosstool-ng`
from source. Download the latest version from http://crosstool-ng.org. The
latest version usually supports the latest released compilers.

.. code-block:: console

   $ wget
   http://crosstool-ng.org/download/crosstool-ng/crosstool-ng-1.22.0.tar.bz2

   $ tar xvf crosstool-ng-1.22.0.tar.bz2

   $ cd crosstool-ng/

   $ ./configure

   $ make

   $ make install


Building the compiler requires a case-senstive file system. Therefore, use
:program:`diskutil` to create an 8 GB blank sparse image making sure you select
case-senstive file system (OS X Extended (Case-sensitive, Journaled) and
mount it.

Alternatively you can use the script below to create the image:

.. code-block:: bash

   #!/bin/bash ImageName=CrossToolNG ImageNameExt=${ImageName}.sparseimage
   diskutil umount force /Volumes/${ImageName} && true rm -f ${ImageNameExt}
   && true hdiutil create ${ImageName} -volname ${ImageName} -type SPARSE
   -size 8g -fs HFSX hdiutil mount ${ImageNameExt} cd /Volumes/$ImageName


When mounted, the file system of the image will be available under
:file:`/Volumes`. Change to the mounted directory:

.. code-block:: console

   $ cd /Volumes/CrossToolNG

   $ mkdir build

   $ cd build

In the Zephyr kernel source tree we provide two configurations for
both ARM and X86 that can be used to pre-select the options needed
for building the toolchain.
The configuration files can be found in :file:`${ZEPHYR_BASE}/scripts/cross_compiler/`.

.. code-block:: console

   $ cp ${ZEPHYR_BASE}/scripts/cross_compiler/x86.config .config

You can create a toolchain configuration or customize an existing configuration
yourself using the configuration menus:

.. code-block:: console

   $ ct-ng menuconfig

To build a kernel image for your desired architecture and platform, perform
these steps:

1. Select the architecture for which you are building the kernel.
2. Select the target platform on which that kernel will run.
3. Save the configuration.
4. Build the toolchain with the saved configuration.
5. Ensure that the build and installation directories are set correctly.
6. Open the generated :file:`.config` file and verify the following (assuming
   the sparse image was mounted under :file:`/Volumes/CrossToolNG`):


.. code-block:: bash

   ...
   CT_LOCAL_TARBALLS_DIR="/Volumes/CrossToolNG/src"
   # CT_SAVE_TARBALLS is not set
   CT_WORK_DIR="${CT_TOP_DIR}/.build"
   CT_PREFIX_DIR="/Volumes/CrossToolNG/x-tools/${CT_TARGET}"
   CT_INSTALL_DIR="${CT_PREFIX_DIR}"
   ...

Now you are ready to build the toolchain:

.. code-block:: console
   $ ct-ng build

The above process takes a while. When finished, the toolchain will be available
under :file:`/Volumes/CrossToolNG/x-tools`.

Repeat the step for all architectures you want to support in your environment.

To use the toolchain with Zephyr, export the following environment variables
and use the target location where the toolchain was installed, type:

.. code-block:: console

   $ export ZEPHYR_GCC_VARIANT=xtools

   $ export ZEPHYR_SDK_INSTALL_DIR=/Volumes/CrossToolNG/x-tools

