.. _installation_linux:

Development Environment Setup on Linux
######################################

This section describes how to set up a Linux development system.

After completing these steps, you will be able to compile and run your Zephyr
applications on the following Linux distributions:

* Ubuntu 14.04 LTS 64-bit
* Fedora 22 64-bit

Where needed, alternative instructions are listed for Ubuntu and Fedora.

Installing the Host's Operating System
**************************************

Building the project's software components including the kernel has been
tested on Ubuntu and Fedora systems. Instructions for installing these OSes
are beyond the scope of this document.

Update Your Operating System
****************************

Before proceeding with the build, ensure your OS is up to date. On Ubuntu:

.. code-block:: console

   $ sudo apt-get update

On Fedora:

.. code-block:: console

   $ sudo dnf update

.. _linux_required_software:

Installing Requirements and Dependencies
****************************************

Install the following with either apt-get or dnf.

.. note::
   Minor version updates of the listed required packages might also
   work.

.. attention::
   Check your firewall and proxy configurations to ensure that Internet
   access is available before attempting to install the required packages.

Install the required packages in a Ubuntu host system with:

.. code-block:: console

   $ sudo apt-get install git make gcc gcc-multilib g++ libc6-dev-i386 \
     g++-multilib

Install the required packages in a Fedora host system with:

.. code-block:: console

   $ sudo dnf group install "Development Tools"
   $ sudo dnf install git make gcc glib-devel.i686 glib2-devel.i686 \
     glibc-static libstdc++-static glibc-devel.i686

.. important::
   Ensure that at least the 32 bit versions of the packages are installed.
   Ideally, both the 32 and 64 bit versions should be installed.

.. _environment_variables:

Setting the Project's Environment Variables
===========================================

#. Navigate to the main project directory:

    .. code-block:: console

       $ cd zephyr-project

#. Source the project environment file to set the project environtment
   variables:

    .. code-block:: console

       $ source zephyr-env.sh

.. _zephyr_sdk:

Installing the Zephyr Software Development Kit
==============================================

Zephyr's :abbr:`SDK (Software Development Kit)` contains all necessary tools
and cross-compilers needed to build the kernel on all supported
architectures. Additionally, it includes host tools such as a custom QEMU and
a host compiler for building host tools if necessary. The SDK supports the
following architectures:

* :abbr:`IA-32 (Intel Architecture 32 bits)`

* :abbr:`ARM (Advanced RISC Machines)`

* :abbr:`ARC (Argonaut RISC Core)`

Follow these steps to install the SDK on your Linux host system.

#. Download the latest SDK self-extractable binary.

    Visit the `Zephyr SDK archive`_ to find all available SDK versions,
    including the latest version.

    Alternatively, you can use the following command to download the desired
    version:

    .. code-block:: console

      $ wget https://nexus.zephyrproject.org/content/repositories/releases/org/zephyrproject/zephyr-sdk/<version>-i686/zephyr-sdk-<version>-i686-setup.run

   .. note::
      Replace <version> with the version number you wish to download.

#. Run the installation binary, follow this example:

    .. code-block:: console

       $ chmod +x zephyr-sdk-<version>-i686-setup.run

       $ sudo ./zephyr-sdk-<version>-i686-setup.run

   .. note::
      Replace <version> with the version number of the SDK that you downloaded.

   .. note::
      There is no need for `sudo` if the SDK is installed in the current
      user's home directory.

#. Follow the installation instructions on the screen. The
   toolchain's default installation location is :file:`/opt/zephyr-sdk/`.

    .. code-block:: console

       Verifying archive integrity... All good.

       Uncompressing SDK for Zephyr  100%

       Enter target directory for SDK (default: /opt/zephyr-sdk/):

#. Enter a new location or hit :kbd:`Return` to accept default.

    .. code-block:: console

       Installing SDK to /opt/zephyr-sdk/

       Creating directory /opt/zephyr-sdk/

       Success

       [*] Installing x86 tools...

       [*] Installing arm tools...

       [*] Installing arc tools...

       ...

       [*] Installing additional host tools...

       Success installing SDK. SDK is ready to be used.

#. To use the Zephyr SDK, export the following environment variables and
   use the target location where SDK was installed, type:

    .. code-block:: console

       $ export ZEPHYR_GCC_VARIANT=zephyr

       $ export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk

  To use the same toolchain in new sessions in the future you can set the
  variables in the file :file:`${HOME}/.zephyrrc`, for example:

  .. code-block:: console

     $ cat <<EOF > ~/.zephyrrc
     export ZEPHYR_GCC_VARIANT=zephyr
     export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk
     EOF

.. _Zephyr SDK archive:
   https://nexus.zephyrproject.org/content/repositories/releases/org/zephyrproject/zephyr-sdk/
