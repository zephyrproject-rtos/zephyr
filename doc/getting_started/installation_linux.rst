.. _installation_linux:

Development Environment Setup on Linux
######################################

This section describes how to set up a Linux development system.

After completing these steps, you will be able to compile and run your Zephyr
applications on the following Linux distributions:

* Ubuntu 16.04 LTS 64-bit
* Fedora 25 64-bit

Where needed, alternative instructions are listed for Ubuntu and Fedora.

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

   $ sudo apt-get update
   $ sudo apt-get upgrade

On Fedora:

.. code-block:: console

   $ sudo dnf upgrade

Note that having a newer version available for an installed package
(and reported by ``dnf check-update``) does not imply a subsequent
``dnf upgrade`` will install it, because it must also ensure dependencies
and other restrictions are satisfied.

.. _linux_required_software:

Installing Requirements and Dependencies
****************************************

Install the following with either apt-get or dnf.

Install the required packages in a Ubuntu host system with:

.. code-block:: console

   $ sudo apt-get install git make gcc g++ python3-ply ncurses-dev \
	 python3-yaml dfu-util dtc

Install the required packages in a Fedora host system with:

.. code-block:: console

   $ sudo dnf group install "Development Tools"
   $ sudo dnf install git make gcc glibc-static \
	 libstdc++-static python3-ply ncurses-devel \
	 python-yaml dfu-util dtc

.. _zephyr_sdk:

Installing the Zephyr Software Development Kit
==============================================

Zephyr's :abbr:`SDK (Software Development Kit)` contains all necessary tools
and cross-compilers needed to build the kernel on all supported
architectures. Additionally, it includes host tools such as a custom QEMU and
a host compiler for building host tools if necessary. The SDK supports the
following architectures:

* :abbr:`X86 (Intel Architecture 32 bits)`

* :abbr:`X86 IAMCU ABI (Intel Architecture 32 bits IAMCU ABI)`

* :abbr:`ARM (Advanced RISC Machines)`

* :abbr:`ARC (Argonaut RISC Core)`

* :abbr:`NIOS II`

* :abbr:`Xtensa`

Follow these steps to install the SDK on your Linux host system.

#. Download the latest SDK self-extractable binary.

   Visit the `Zephyr SDK archive`_ to find all available SDK versions,
   including the latest version.

   Alternatively, you can use the following command to download the
   desired version (*0.9.1* can be replaced with the version number you
   wish to download).

   .. code-block:: console

      $ wget https://github.com/zephyrproject-rtos/meta-zephyr-sdk/releases/download/0.9.1/zephyr-sdk-0.9.1-setup.run

#. Run the installation binary, follow this example:

   .. important::
      Make sure you have installed all required packages for your host
      distribution as described in the previous section
      `linux_required_software`_ otherwise the SDK installation will fail.

   .. code-block:: console

      $ chmod +x zephyr-sdk-<version>-setup.run
      $ ./zephyr-sdk-<version>-setup.run

   There is no need for `sudo` if the SDK is installed in the current
   user's home directory.

#. Follow the installation instructions on the screen. The
   toolchain's default installation location is :file:`/opt/zephyr-sdk/`.
   To install in the default installation location, you will need to use sudo. It is recommended
   to install the SDK in your home directory and not in a system directory.

#. To use the Zephyr SDK, export the following environment variables and
   use the target location where SDK was installed, type:

   .. code-block:: console

      $ export ZEPHYR_GCC_VARIANT=zephyr
      $ export ZEPHYR_SDK_INSTALL_DIR=<sdk installation directory>

  To use the same toolchain in new sessions in the future you can set the
  variables in the file :file:`${HOME}/.zephyrrc`, for example:

  .. code-block:: console

     $ cat <<EOF > ~/.zephyrrc
     export ZEPHYR_GCC_VARIANT=zephyr
     export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk
     EOF

.. _Zephyr SDK archive:
    https://zephyrproject.org/downloads/tools
