.. _installation_linux:

Development Environment Setup on Linux
######################################

This section describes how to set up a Linux development system.

After completing these steps, you will be able to compile and run your Zephyr
applications on the following Linux distributions:

* Ubuntu 16.04 LTS 64-bit
* Fedora 25 64-bit
* Clear Linux
* Arch Linux (install `zephyr-sdk <https://aur.archlinux.org/packages/zephyr-sdk>`_ package from AUR)

Where needed, alternative instructions are listed for specific Linux
distributions.

Installing the Host's Operating System
**************************************

Building the project's software components including the kernel has been
tested on Ubuntu and Fedora systems. Instructions for installing these OSes
are beyond the scope of this document.

Update Your Operating System
****************************

Before proceeding with the build, ensure your OS is up to date.  On Ubuntu,
you'll first need to update the local database list of available packages
before upgrading:

.. code-block:: console

   sudo apt-get update
   sudo apt-get upgrade

On Fedora:

.. code-block:: console

   sudo dnf upgrade

Note that having a newer version available for an installed package
(and reported by ``dnf check-update``) does not imply a subsequent
``dnf upgrade`` will install it, because it must also ensure dependencies
and other restrictions are satisfied.

On Clear Linux:

.. code-block:: console

   sudo swupd update

Installing Requirements and Dependencies
****************************************

Install the following required packages using either apt-get or dnf.

On Ubuntu host system:

.. code-block:: console

   sudo apt-get install --no-install-recommends git cmake ninja-build gperf \
     ccache doxygen dfu-util device-tree-compiler \
     python3-ply python3-pip python3-setuptools python3-wheel xz-utils file \
     make gcc-multilib autoconf automake libtool librsvg2-bin \
     texlive-latex-base texlive-latex-extra latexmk texlive-fonts-recommended

On Fedora host system:

.. code-block:: console

   sudo dnf group install "Development Tools" "C Development Tools and Libraries"
   sudo dnf install git cmake ninja-build gperf ccache\
     doxygen dfu-util dtc python3-pip \
     python3-ply python3-yaml dfu-util dtc python3-pykwalify \
     glibc-devel.i686 libstdc++-devel.i686 autoconf automake libtool \
     texlive-latex latexmk texlive-collection-fontsrecommended librsvg2-tools

On Clear Linux host system:

.. code-block:: console

   sudo swupd bundle-add c-basic dev-utils dfu-util dtc \
     os-core-dev python-basic python3-basic texlive

Install additional packages required for development with Zephyr::

   cd ~/zephyr  # or to your directory where zephyr is cloned
   pip3 install --user -r scripts/requirements.txt

CMake version 3.8.2 or higher is required. Check what version you have using
``cmake --version``; if you have an older version, check the backports or
install a more recent version manually. For example, to install version
3.8.2 from the CMake website directly in ~/cmake::

   mkdir $HOME/cmake && cd $HOME/cmake
   wget https://cmake.org/files/v3.8/cmake-3.8.2-Linux-x86_64.sh
   yes | sh cmake-3.8.2-Linux-x86_64.sh | cat
   echo "export PATH=$PWD/cmake-3.8.2-Linux-x86_64/bin:\$PATH" >> $HOME/.zephyrrc
   source <zephyr git clone location>/zephyr-env.sh
   cmake --version

.. _zephyr_sdk:

Installing the Zephyr Software Development Kit
==============================================

Zephyr's :abbr:`SDK (Software Development Kit)` contains all necessary tools
and cross-compilers needed to build the kernel on all supported
architectures. Additionally, it includes host tools such as custom QEMU binaries
and a host compiler for building host tools if necessary. The SDK supports the
following architectures:

* :abbr:`X86 (Intel Architecture 32 bits)`

* :abbr:`X86 IAMCU ABI (Intel Architecture 32 bits IAMCU ABI)`

* :abbr:`ARM (Advanced RISC Machines)`

* :abbr:`ARC (Argonaut RISC Core)`

* :abbr:`Nios II`

* :abbr:`Xtensa`

* :abbr:`RISC-V`

Follow these steps to install the SDK on your Linux host system.

#. Download the latest SDK as a self-extracting installation binary:

   .. code-block:: console

      wget https://github.com/zephyrproject-rtos/meta-zephyr-sdk/releases/download/0.9.3/zephyr-sdk-0.9.3-setup.run

   (You can change *0.9.3* to another version if needed; the `Zephyr
   Downloads`_ page contains all available SDK releases.)

#. Run the installation binary:

   .. code-block:: console

      cd <sdk download directory>
      sh zephyr-sdk-0.9.3-setup.run

   .. important::
      If this fails, make sure Zephyr's dependencies were installed
      as described in `Installing Requirements and Dependencies`_.

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

  To use the same toolchain in new sessions in the future, you can set the
  variables in the file :file:`${HOME}/.zephyrrc`, for example:

  .. code-block:: console

     cat <<EOF > ~/.zephyrrc
     export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
     export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk
     EOF

  .. note::
     Use ``<sdk installation directory>`` in place of ``/opt/zephyr-sdk/`` in the
     above shown example if the SDK installation location is not default.


.. note:: In previous releases of Zephyr, the ``ZEPHYR_TOOLCHAIN_VARIANT``
          variable was called ``ZEPHYR_GCC_VARIANT``.

.. _Zephyr Downloads:
    https://www.zephyrproject.org/developers/#downloads
