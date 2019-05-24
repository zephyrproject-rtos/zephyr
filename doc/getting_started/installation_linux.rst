.. _installation_linux:

Development Environment Setup on Linux
######################################

.. important::

   This section only describes OS-specific setup instructions; it is the first step in the
   complete Zephyr :ref:`getting_started`.

This section describes how to set up a Zephyr development environment on the
following Linux distributions:

* Ubuntu 16.04 LTS or 18.04 LTS 64-bit
* Fedora 28 64-bit
* Clear Linux
* Arch Linux

Where needed, instructions are given which only apply to specific Linux
distributions.

Update Your Operating System
****************************

Ensure your host system is up to date before proceeding.

On Ubuntu:

.. code-block:: console

   sudo apt-get update
   sudo apt-get upgrade

On Fedora:

.. code-block:: console

   sudo dnf upgrade

Note that having a newer version available for an installed package
(as reported by ``dnf check-update``) does not imply a subsequent
``dnf upgrade`` will install it, because it must also ensure dependencies
and other restrictions are satisfied.

On Clear Linux:

.. code-block:: console

   sudo swupd update

On Arch Linux:

.. code-block:: console

   sudo pacman -Syu

.. _linux_requirements:

Install Requirements and Dependencies
*************************************

.. NOTE FOR DOCS AUTHORS: DO NOT PUT DOCUMENTATION BUILD DEPENDENCIES HERE.

   This section is for dependencies to build Zephyr binaries, *NOT* this
   documentation. If you need to add a dependency only required for building
   the docs, add it to doc/README.rst. (This change was made following the
   introduction of LaTeX->PDF support for the docs, as the texlive footprint is
   massive and not needed by users not building PDF documentation.)

Install the following packages using your system's package manager. Note that
both Ninja and Make are installed; you may prefer only to install one.

On Ubuntu:

.. code-block:: console

   sudo apt-get install --no-install-recommends git cmake ninja-build gperf \
     ccache dfu-util device-tree-compiler wget \
     python3-pip python3-setuptools python3-wheel xz-utils file make gcc \
     gcc-multilib

On Fedora:

.. code-block:: console


   sudo dnf group install "Development Tools" "C Development Tools and Libraries"
   dnf install git cmake ninja-build gperf ccache dfu-util dtc wget \
     python3-pip xz file glibc-devel.i686 libstdc++-devel.i686

On Clear Linux:

.. code-block:: console

   sudo swupd bundle-add c-basic dev-utils dfu-util dtc \
     os-core-dev python-basic python3-basic

On Arch:

.. code-block:: console

   sudo pacman -S git cmake ninja gperf ccache dfu-util dtc wget \
       python-pip python-setuptools python-wheel xz file make

.. important::
   Zephyr requires a recent version of CMake. Read through
   the rest of the section below to verify the version you have
   installed is recent enough to build Zephyr.

CMake version 3.13.1 or higher is required. Check what version you have by using
``cmake --version``. If you have an older version, there are several ways
of obtaining a more recent one:

* Use ``pip``:

  .. code-block:: console

     pip3 install --user cmake

* Download and install from the pre-built binaries provided by the CMake
  project itself in the `CMake Downloads`_ page.
  For example, to install version 3.13.1 in :file:`~/bin/cmake`:

  .. code-block:: console

     mkdir $HOME/bin/cmake && cd $HOME/bin/cmake
     wget https://github.com/Kitware/CMake/releases/download/v3.13.1/cmake-3.13.1-Linux-x86_64.sh
     yes | sh cmake-3.13.1-Linux-x86_64.sh | cat
     echo "export PATH=$PWD/cmake-3.13.1-Linux-x86_64/bin:\$PATH" >> $HOME/.zephyrrc

* Check your distribution's beta or unstable release package library for an
  update.

.. note::
   If you have installed a recent version of CMake using one of the approaches
   listed above, you might want to uninstall the one provided by your
   distribution's package manager (``apt``, ``dnf``, ``swupd``, ``pacman``,
   etc.) in order to avoid version conflicts.

.. _zephyr_sdk:

Install the Zephyr Software Development Kit (SDK)
*************************************************

.. note::

   Use of the Zephyr SDK is optional, but recommended. Some of the requirements
   and dependencies in the previous section are only needed for installing the
   SDK.

Zephyr's :abbr:`SDK (Software Development Kit)` contains all necessary tools
and cross-compilers needed to build Zephyr on all supported
architectures. Additionally, it includes host tools such as custom QEMU binaries
and a host compiler for building host tools if necessary. The SDK supports the
following target architectures:

* :abbr:`X86 (Intel Architecture 32 bits)`

* :abbr:`X86 IAMCU ABI (Intel Architecture 32 bits IAMCU ABI)`

* :abbr:`Arm (Advanced RISC Machine)`

* :abbr:`ARC (Argonaut RISC Core)`

* :abbr:`Nios II`

* :abbr:`Xtensa`

* :abbr:`RISC-V`

Follow these steps to install the SDK on your Linux host system.

#. Download the latest SDK as a self-extracting installation binary:

   .. code-block:: console

      wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.10.0/zephyr-sdk-0.10.0-setup.run

   (You can change *0.10.0* to another version if needed; the `Zephyr
   Downloads`_ page contains all available SDK releases.)

#. Run the installation binary:

   .. code-block:: console

      cd <sdk download directory>
      sh zephyr-sdk-0.10.0-setup.run

   .. important::
      If this fails, make sure Zephyr's dependencies were installed
      as described in `Install Requirements and Dependencies`_.

#. Follow the installation instructions on the screen. The toolchain's
   default installation location is :file:`/opt/zephyr-sdk/`, but it
   is recommended to install the SDK under your home directory instead.

   To install the SDK in the default location, you need to run the
   installation binary as root.

#. To use the Zephyr SDK, export the following environment variables and
   use the target location where SDK was installed:

   .. code-block:: console

      export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
      export ZEPHYR_SDK_INSTALL_DIR=<sdk installation directory>

.. _sdkless_builds:

Building on Linux without the Zephyr SDK
****************************************

The Zephyr SDK is provided for convenience and ease of use. It provides
toolchains for all Zephyr target architectures, and does not require any extra
flags when building applications or running tests. In addition to
cross-compilers, the Zephyr SDK also provides prebuilt host tools. It is,
however, possible to build without the SDK's toolchain by using another
toolchain as as described in the main :ref:`getting_started` document.

As already noted above, the SDK also includes prebuilt host tools.  To use the
SDK's prebuilt host tools with a toolchain from another source, keep the
:envvar:`ZEPHYR_SDK_INSTALL_DIR` environment variable set to the Zephyr SDK
installation directory. To build without the Zephyr SDK's prebuilt host tools,
the :envvar:`ZEPHYR_SDK_INSTALL_DIR` environment variable must be unset before
you run ``source zephyr-env.sh`` later on in the Getting Started Guide.

To make sure this variable is unset, run:

.. code-block:: console

   unset ZEPHYR_SDK_INSTALL_DIR

.. _Zephyr Downloads: https://www.zephyrproject.org/developers/#downloads
.. _CMake Downloads: https://cmake.org/download
