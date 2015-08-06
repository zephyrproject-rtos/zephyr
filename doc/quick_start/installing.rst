.. _installing_zephyr:

Installing the Zephyr Kernel
############################

This section describes all the installation procedures needed to install the
kernel in a development system and how to get access to the kernel's source
code.

.. _linux_development_system:

Using a Linux Development System
********************************

Installing the Operating System
===============================

The steps needed for installing the operating system of the development
system are beyond the scope of this document. Please refer to the
documentation of your operating system of choice. The project supports both
Ubuntu and Fedora.

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

.. _required_software:

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

.. _yocto_sdk:

Installing the Yocto Software Development Kit
=============================================

The |project|'s :abbr:`SDK (Software Development Kit)` provided by
Yocto contains all necessary tools and cross compilers needed to build the
|codename| on all supported architectures. In addition it includes
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

Installing a Custom QEMU for ARM Platforms
============================================

The Yocto SDK comes with a Qemu binary suitable for running sample |codename|
applications. The steps below are only needed if you choose not to use the
provided binary and use a custom built binary instead.

If you require to test ARM builds, a localized patch to the QEMU source
is needed. The patch corrects the issues with the locking interfaces
QEMU uses. If you are working only with the x86 builds of the Zephyr kernel,
install QEMU from your systems default package manager.

Follow these steps to enable a customized build of QEMU:

#. Clone the QEMU repository, type:

.. code-block:: bash

   $ git clone git://git.qemu-project.org/qemu.git

#. Checkout the v2.1 stable branch, type:

.. code-block:: bash

   $ cd qemu

   $ git checkout stable-2.1

#. Apply our internal patch, type:

.. code-block:: bash

   $ git am $ZEPHYR_BASE/scripts/0001-armv7m-support-basepri-primask-
   interrupt-locking.patch

#. Update the submodules as needed, type:

.. code-block:: bash

   $ git submodule update --init pixman

   $ git submodule update --init dtc

#. Build QEMU v2.1, type:

.. code-block:: bash

   $ ./configure && make

* You can also build QEMU to a private directory, type:

.. code-block:: bash

   $ ./configure --prefix=$MY_PREFERED_INSTALL_LOCATION && make

* Install QEMU, type:

.. code-block:: bash

   $ sudo make install

.. _setup_development_environment:

Setup a Local Development Environment
**************************************

The |project|'s source code is maintained using GIT and is served using
Gerrit.

Gerrit access requires some basic user setup. The following process has
been defined as a simple walk-through to enable quick access to the
Gerrit services.

.. _access_source:

Getting Access
================

#. `Create`_ or `update`_ a 01.org_ account.

#. Submit your your 01.org_ account and corporate email address to
   |PM| `<mailto:hirally.santiago.rodriguez@intel.com>`_.

#. Once access is granted, `access Gerrit`_.

#. Log in using your 01.org account credentials.

.. _Create: https://01.org/user/register

.. _update: https://01.org/user/login

.. _access Gerrit: https://oic-review.01.org/gerrit/

.. _01.org: https://01.org/

Configuring SSH to Use Gerrit
=============================

Gerrit uses SSH to interact with your GIT client. A SSH private key
needs to be generated on the development machine with a matching public
key on the Gerrit server.

If you already have a SSH key-pair you would like to use, please skip
down to step.

Please follow the steps below to get started.

1. Create a key-pair in your Linux machine, type:

.. code-block:: bash

   $ ssh-keygen -t rsa -C "John Doe john.doe@example.com"

.. note:: This will ask you for a password to protect the private key as it
   generates a unique key. Please keep this password private, and DO
   NOT enter a blank password.


The generated key-pair is found at:
:file:`~/.ssh/id_rsa and ~/.ssh/id_rsa.pub`.

2. Add the the private key in the :file:`id_rsa` file in your key ring,
type:

.. code-block:: bash

   $ ssh-add ~/.ssh/id_rsa

3. Add your the public key :file:`id_rsa.pub` to the Gerrit account:

   a. Go to `access Gerrit`_.

   b. Click on your account name in the upper right corner.

   c. From the pop-up menu, select :guilabel:`Settings`.

   d. On the left hand menu select, click on
   :guilabel:`SSH Public Keys`.

   e. Click Add key and paste the contents of your public key
   :file:`~/.id/id_rsa.pub`.

.. note:: To obtain the contents of your public key on a Linux machine type:

   :command:`$ cat ~/.ssh/id_rsa.pub`

   The output is the contents of :file:`~/.id/id_rsa.pub`. Paste it into the
  Add SSH key window in Gerrit.

.. warning:: Potential Security Risk
   Do not copy your private key :file:`~/.ssh/id_rsa` Use only the public
   :file:`~/.id/id_rsa.pub`.

.. _checking_source_out:

Checking Out the Source Code
============================

#. Ensure that SSH has been set up porperly. See
   `Configuring SSH to Use Gerrit`_ for details.

#. Clone the repository, type:

   .. code-block:: bash

      $ git clone ssh://01ORGUSERNAME@oic-review.01.org:29418/forto-collab

#. You have checked out a local copy of the source code. Develop
   freely, issuing as many commits and rebases as needed.


#. Change to the main project directory, type:

    .. code-block:: bash

       $ cd forto-collab

#. Source the project environment file to setup project variables, type:

    .. code-block:: bash

       $ source zephyr-env.sh