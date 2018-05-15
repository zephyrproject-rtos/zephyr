.. _installing_zephyr_mac:

Development Environment Setup on macOS
######################################

This section describes how to set up a macOS development system.

After completing these steps, you will be able to compile and run your Zephyr
applications on the following macOS version:

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

After Homebrew is successfully installed, install the following tools using
the brew command line.

.. note::
   Zephyr requires Python 3 in order to be built. Since macOS comes bundled
   only with Python 2, we will need to install Python 3 with Homebrew. After
   installing it you should have the macOS-bundled Python 2 in ``/usr/bin/``
   and the Homebrew-provided Python 3 in ``/usr/local/bin``.

Install tools to build Zephyr binaries:

.. code-block:: console

   brew install cmake ninja dfu-util doxygen qemu dtc python3 gperf
   cd ~/zephyr   # or to the folder where you cloned the zephyr repo
   pip3 install --user -r scripts/requirements.txt

.. note::
   If ``pip3`` does not seem to have been installed correctly use
   ``brew reinstall python3`` in order to reinstall it.

If you require pyocd, an open source Python 2 library for programming and
debugging ARM Cortex-M microcontrollers, use this command to install it using
the macOS-bundled Python 2:

.. code-block:: console

   pip install --user -r scripts/py2-requirements.txt

If pip for the macOS-bundled Python 2 is not installed, you can install it with:

.. code-block:: console

   sudo python -m ensurepip

Source :file:`zephyr-env.sh` wherever you have cloned the Zephyr Git repository:

.. code-block:: console

   unset ZEPHYR_SDK_INSTALL_DIR
   cd <zephyr git clone location>
   source zephyr-env.sh

Finally, assuming you are using a 3rd-party toolchain you can try building the :ref:`hello_world` sample to check things out.

To build for the ARM-based Nordic nRF52 Development Kit:

.. zephyr-app-commands::
  :zephyr-app: samples/hello_world
  :board: nrf52_pca10040
  :host-os: unix
  :goals: build

.. _setting_up_mac_toolchain:

Setting Up the Toolchain
************************

Install tools needed for building the toolchain (if needed):

.. code-block:: console

   brew install gettext help2man mpfr gmp coreutils wget
   brew tap homebrew/dupes
   brew install grep --with-default-names


To build the toolchain, you will need crosstool-ng version 1.23 or higher.
Install it by using Homebrew:

.. code-block:: console

   brew install crosstool-ng

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

   cd /Volumes/CrossToolNG
   mkdir build
   cd build

Setting the Toolchain Options
=============================

In the Zephyr kernel source tree we provide configurations for NIOS-II and
X86 that can be used to preselect the options needed for building the toolchain.

The configuration files can be found in
:file:`${ZEPHYR_BASE}/scripts/cross_compiler/`.

Currently the following configurations are provided:

* i586.config: for standard ABI, for example for Galileo and qemu_x86
* iamcu.config: for IAMCU ABI, for example for the Arduino 101
* nios2.config: for Nios II boards

.. code-block:: console

   cp ${ZEPHYR_BASE}/scripts/cross_compiler/i586.config .config

You can create a toolchain configuration or customize an existing configuration
yourself using the configuration menus:

.. code-block:: console

   export CT_PREFIX=/Volumes/CrossToolNG
   ct-ng oldconfig

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

   ct-ng build

The above process takes a while. When finished, the toolchain will be available
under :file:`/Volumes/CrossToolNG/x-tools`.

Repeat the step for all architectures you want to support in your environment.

To use the toolchain with Zephyr, export the following environment variables
and use the target location where the toolchain was installed, type:

.. code-block:: console

   export ZEPHYR_TOOLCHAIN_VARIANT=xtools
   export XTOOLS_TOOLCHAIN_PATH=/Volumes/CrossToolNG/x-tools


To use the same toolchain in new sessions in the future you can set the
variables in the file :file:`${HOME}/.zephyrrc`, for example:

.. code-block:: console

   cat <<EOF > ~/.zephyrrc
   export XTOOLS_TOOLCHAIN_PATH=/Volumes/CrossToolNG/x-tools
   export ZEPHYR_TOOLCHAIN_VARIANT=xtools
   EOF

.. note:: In previous releases of Zephyr, the ``ZEPHYR_TOOLCHAIN_VARIANT``
          variable was called ``ZEPHYR_GCC_VARIANT``.

.. _Homebrew site: http://brew.sh/

.. _crosstool-ng site: http://crosstool-ng.org
