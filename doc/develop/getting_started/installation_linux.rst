.. _installation_linux:

Install Linux Host Dependencies
###############################

Documentation is available for these Linux distributions:

* Ubuntu
* Fedora
* Clear Linux
* Arch Linux

For distributions that are not based on rolling releases, some of the
requirements and dependencies may not be met by your package manager. In that
case please follow the additional instructions that are provided to find
software from sources other than the package manager.

.. note:: If you're working behind a corporate firewall, you'll likely
   need to configure a proxy for accessing the internet, if you haven't
   done so already.  While some tools use the environment variables
   ``http_proxy`` and ``https_proxy`` to get their proxy settings, some
   use their own configuration files, most notably ``apt`` and
   ``git``.

Update Your Operating System
****************************

Ensure your host system is up to date.

.. tabs::

   .. group-tab:: Ubuntu

      .. code-block:: console

         sudo apt-get update
         sudo apt-get upgrade

   .. group-tab:: Fedora

      .. code-block:: console

         sudo dnf upgrade

   .. group-tab:: Clear Linux

      .. code-block:: console

         sudo swupd update

   .. group-tab:: Arch Linux

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

Note that both Ninja and Make are installed with these instructions; you only
need one.

.. tabs::

   .. group-tab:: Ubuntu

      .. code-block:: console

         sudo apt-get install --no-install-recommends git cmake ninja-build gperf \
           ccache dfu-util device-tree-compiler wget \
           python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file libpython3.8-dev \
           make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

   .. group-tab:: Fedora

      .. code-block:: console

         sudo dnf group install "Development Tools" "C Development Tools and Libraries"
         sudo dnf install git cmake ninja-build gperf ccache dfu-util dtc wget \
           python3-pip python3-tkinter xz file glibc-devel.i686 libstdc++-devel.i686 python38 \
           SDL2-devel

   .. group-tab:: Clear Linux

      .. code-block:: console

         sudo swupd bundle-add c-basic dev-utils dfu-util dtc \
           os-core-dev python-basic python3-basic python3-tcl

      The Clear Linux focus is on *native* performance and security and not
      cross-compilation. For that reason it uniquely exports by default to the
      :ref:`environment <env_vars>` of all users a list of compiler and linker
      flags. Zephyr's CMake build system will either warn or fail because of
      these. To clear the C/C++ flags among these and fix the Zephyr build, run
      the following command as root then log out and back in:

      .. code-block:: console

         echo 'unset CFLAGS CXXFLAGS' >> /etc/profile.d/unset_cflags.sh

      Note this command unsets the C/C++ flags for *all users on the
      system*. Each Linux distribution has a unique, relatively complex and
      potentially evolving sequence of bash initialization files sourcing each
      other and Clear Linux is no exception. If you need a more flexible
      solution, start by looking at the logic in
      ``/usr/share/defaults/etc/profile``.

   .. group-tab:: Arch Linux

      .. code-block:: console

         sudo pacman -S git cmake ninja gperf ccache dfu-util dtc wget \
             python-pip python-setuptools python-wheel tk xz file make

CMake
=====

A :ref:`recent CMake version <install-required-tools>` is required. Check what
version you have by using ``cmake --version``. If you have an older version,
there are several ways of obtaining a more recent one:

* On Ubuntu, you can follow the instructions for adding the
  `kitware third-party apt repository <https://apt.kitware.com/>`_
  to get an updated version of cmake using apt.

* Download and install a packaged cmake from the CMake project site.
  (Note this won't uninstall the previous version of cmake.)

  .. code-block:: console

     cd ~
     wget https://github.com/Kitware/CMake/releases/download/v3.21.1/cmake-3.21.1-Linux-x86_64.sh
     chmod +x cmake-3.21.1-Linux-x86_64.sh
     sudo ./cmake-3.21.1-Linux-x86_64.sh --skip-license --prefix=/usr/local
     hash -r

  The ``hash -r`` command may be necessary if the installation script
  put cmake into a new location on your PATH.

* Download and install from the pre-built binaries provided by the CMake
  project itself in the `CMake Downloads`_ page.
  For example, to install version 3.21.1 in :file:`~/bin/cmake`:

  .. code-block:: console

     mkdir $HOME/bin/cmake && cd $HOME/bin/cmake
     wget https://github.com/Kitware/CMake/releases/download/v3.21.1/cmake-3.21.1-Linux-x86_64.sh
     yes | sh cmake-3.21.1-Linux-x86_64.sh | cat
     echo "export PATH=$PWD/cmake-3.21.1-Linux-x86_64/bin:\$PATH" >> $HOME/.zephyrrc

* Use ``pip3``:

  .. code-block:: console

     pip3 install --user cmake

  Note this won't uninstall the previous version of cmake and will
  install the new cmake into your ~/.local/bin folder so
  you'll need to add ~/.local/bin to your PATH.  (See :ref:`python-pip`
  for details.)

* Check your distribution's beta or unstable release package library for an
  update.

* On Ubuntu you can also use snap to get the latest version available:

  .. code-block:: console

     sudo snap install cmake

After updating cmake, verify that the newly installed cmake is found
using ``cmake --version``.
You might also want to uninstall the CMake provided by your package manager to
avoid conflicts.  (Use ``whereis cmake`` to find other installed
versions.)

DTC (Device Tree Compiler)
==========================

A :ref:`recent DTC version <install-required-tools>` is required. Check what
version you have by using ``dtc --version``. If you have an older version,
either install a more recent one by building from source, or use the one that is
bundled in the :ref:`Zephyr SDK <toolchain_zephyr_sdk>` by installing it.

Python
======

A :ref:`modern Python 3 version <install-required-tools>` is required. Check
what version you have by using ``python3 --version``.

If you have an older version, you will need to install a more recent Python 3.
You can build from source, or use a backport from your distribution's package
manager channels if one is available. Isolating this Python in a virtual
environment is recommended to avoid interfering with your system Python.

.. _pyenv: https://github.com/pyenv/pyenv

Install the Zephyr Software Development Kit (SDK)
*************************************************

The Zephyr Software Development Kit (SDK) contains toolchains for each of
Zephyr's supported architectures. It also includes additional host tools, such
as custom QEMU and OpenOCD.

Use of the Zephyr SDK is highly recommended and may even be required under
certain conditions (for example, running tests in QEMU for some architectures).

The Zephyr SDK supports the following target architectures:

* ARC (32-bit and 64-bit; ARCv1, ARCv2, ARCv3)
* ARM (32-bit and 64-bit; ARMv6, ARMv7, ARMv8; A/R/M Profiles)
* MIPS (32-bit and 64-bit)
* Nios II
* RISC-V (32-bit and 64-bit; RV32I, RV32E, RV64I)
* x86 (32-bit and 64-bit)
* Xtensa

Follow these steps to install the Zephyr SDK:

#. Download and verify the `latest Zephyr SDK bundle
   <https://github.com/zephyrproject-rtos/sdk-ng/releases>`_:

   .. code-block:: bash

      wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.14.2/zephyr-sdk-0.14.2_linux-x86_64.tar.gz
      wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.14.2/sha256.sum | shasum --check --ignore-missing

   You can change ``0.14.2`` to another version if needed; the `Zephyr SDK
   Releases`_ page contains all available SDK releases.

   If your host architecture is 64-bit ARM (for example, Raspberry Pi), replace
   ``x86_64`` with ``aarch64`` in order to download the 64-bit ARM Linux SDK.

#. Extract the Zephyr SDK bundle archive:

   .. code-block:: bash

      cd <sdk download directory>
      tar xvf zephyr-sdk-0.14.2_linux-x86_64.tar.gz

#. Run the Zephyr SDK bundle setup script:

   .. code-block:: bash

      cd zephyr-sdk-0.14.2
      ./setup.sh

   If this fails, make sure Zephyr's dependencies were installed as described
   in `Install Requirements and Dependencies`_.

If you want to uninstall the SDK, remove the directory where you installed it.
If you relocate the SDK directory, you need to re-run the setup script.

.. note::
   It is recommended to extract the Zephyr SDK bundle at one of the following locations:

   * ``$HOME``
   * ``$HOME/.local``
   * ``$HOME/.local/opt``
   * ``$HOME/bin``
   * ``/opt``
   * ``/usr/local``

   The Zephyr SDK bundle archive contains the ``zephyr-sdk-0.14.2`` directory and, when
   extracted under ``$HOME``, the resulting installation path will be
   ``$HOME/zephyr-sdk-0.14.2``.

   If you install the Zephyr SDK outside any of these locations, you must
   register the Zephyr SDK in the CMake package registry by running the setup
   script, or set :envvar:`ZEPHYR_SDK_INSTALL_DIR` to point to the Zephyr SDK
   installation directory.

   You can also use :envvar:`ZEPHYR_SDK_INSTALL_DIR` for pointing to a
   directory containing multiple Zephyr SDKs, allowing for automatic toolchain
   selection. For example, ``ZEPHYR_SDK_INSTALL_DIR=/company/tools``, where
   the ``company/tools`` folder contains the following subfolders:

   * ``/company/tools/zephyr-sdk-0.13.2``
   * ``/company/tools/zephyr-sdk-a.b.c``
   * ``/company/tools/zephyr-sdk-x.y.z``

   This allows the Zephyr build system to choose the correct version of the
   SDK, while allowing multiple Zephyr SDKs to be grouped together at a
   specific path.

.. _sdkless_builds:

Building on Linux without the Zephyr SDK
****************************************

The Zephyr SDK is provided for convenience and ease of use. It provides
toolchains for all Zephyr target architectures, and does not require any extra
flags when building applications or running tests. In addition to
cross-compilers, the Zephyr SDK also provides prebuilt host tools. It is,
however, possible to build without the SDK's toolchain by using another
toolchain as as described in the :ref:`toolchains` section.

As already noted above, the SDK also includes prebuilt host tools.  To use the
SDK's prebuilt host tools with a toolchain from another source, you must set the
:envvar:`ZEPHYR_SDK_INSTALL_DIR` environment variable to the Zephyr SDK
installation directory. To build without the Zephyr SDK's prebuilt host tools,
the :envvar:`ZEPHYR_SDK_INSTALL_DIR` environment variable must be unset.

To make sure this variable is unset, run:

.. code-block:: console

   unset ZEPHYR_SDK_INSTALL_DIR

.. _Zephyr SDK Releases: https://github.com/zephyrproject-rtos/sdk-ng/releases
.. _CMake Downloads: https://cmake.org/download
