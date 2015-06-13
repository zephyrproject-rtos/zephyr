Using Linux as the Host System
**************************************


Installing the Operating System
=================================

The steps needed for installing the operating system of the host system
are beyond the scope of this document. Please refer to the
documentation of your operating system of choice.  We recommend the use of
Ubuntu or Fedora.

Configuring Network and Proxies
=================================

The installation process requires the use of git, ssh, wget,
curl, and apt-get. The installation and configuration of these services for
a development system is beyond the scope of this document.  Once installed,
verify that each service can access the Internet and is not impeded by a
firewall.

Before you continue, ensure that your development system can use the
above commands in both User and root configurations. Please refer to
the documentation of your operating system of choice.


Update Your Operating System
=================================

In Ubuntu enter:

.. code-block:: bash

   $ sudo apt-get update

In Fedora enter:

.. code-block:: bash

   $ sudo yum update

Installing Required Software
=================================

Install the following requirements on the host system using either
apt-get or yum.

.. note::
   Minor version updates of the listed required packages might also
   work.


Install the required packages in a Ubuntu host system, type:

.. code-block:: bash

   $ sudo apt-get install git make gcc gcc-multilib g++ \
     libc6-dev-i386 g++-multilib

Install the required packages in a Fedora host system, type:

.. code-block:: bash

   $ sudo yum groupinstall "Development Tools"
   $ sudo yum install git make gcc glib-devel.i686 \
     glib2-devel.i686 g++ libc6-dev-i386 g++-multilib \
     glibc-static libstdc++-static

Installing the Yocto Software Development Kit
=============================================

The |project| :abbr:`Software Development Kit (SDK)` provided by
Yocto contains all necessary tools and cross compilers needed to build
Zephyr kernels on all supported architectures. In addition it includes
host tools such as a custom QEMU and a host compiler for building host
tools if necessary. With this SDK, there is no need to build any cross
compilers or emulation environments. The SDK supports the following
architectures:

* :abbr:`IA-32 (Intel Architecture 32 bits)`

* :abbr:`ARM (Advanced RISC Machines)`

* :abbr:`ARC (Argonaut RISC Core)`

Follow these steps to install the SDK on your host system.

#. Download the Yocto self-extractable binary from:

   http://yct-rtos02.ostc.intel.com/tm-toolchains-i686-setup.run

#. Run the installation binary, type:

    .. code-block:: bash

       $ chmod +x tm-toolchains-i686-setup.run

       $ sudo ./tm-toolchains-i686-setup.run


#. Follow the installation instructions on the screen. The
   toolchain's default installation location is :file:`/opt/poky-tm`.

    .. code-block:: bash

       Verifying archive integrity... All good.

       Uncompressing SDK for TM 100%

       Enter target directory for SDK (default: /opt/poky-tm/1.8):

#. Enter a new location or hit :kbd:`Return` to accept default.

    .. code-block:: bash

       Installing SDK to /opt/poky-tm/1.8

       Creating directory /opt/poky-tm/1.8

       Success

       [*] Installing x86 tools...

       [*] Installing arm tools...

       [*] Installing arc tools...

       [*] Installing additional host tools...

       Success installing SDK. SDK is ready to be used.

#. To use the Yocto SDK, export the following environment variables,
   type:

    .. code-block:: bash

       $ export ZEPHYR_GCC_VARIANT=yocto

       $ export YOCTO_SDK_INSTALL_DIR=/opt/poky-tm/1.8




