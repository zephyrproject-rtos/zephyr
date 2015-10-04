.. _installing_zephyr:

Installing the Zephyr Kernel
############################

This section describes how to install the kernel in a development system
and how to get access to the kernel's source code.

.. _linux_development_system:

Prerequisites
*************

Installing the Operating System
===============================

The kernel has been tested on Ubuntu and Fedora. Instructions for
installing these OSes are beyond the scope of this document.

Configuring Network and Proxies
===============================

Building the kernel requires the command-line tools of git, ssh, wget,
curl, and apt-get. Verify that each service can be run as both
user and root and that access to the Internet and is not impeded
by a firewall.

Update Your Operating System
============================

Before proceeding with the build, ensure your OS is up to date. On Ubuntu:

.. code-block:: bash

   $ sudo apt-get update

On Fedora:

.. code-block:: bash

   $ sudo dnf update

.. _required_software:

Installing Requirements and Dependencies
========================================

Install the following with either apt-get or yum.

.. note::
   Minor version updates of the listed required packages might also
   work.

Install the required packages in a Ubuntu host system with:

.. code-block:: bash

   $ sudo apt-get install git make gcc gcc-multilib g++ \
     libc6-dev-i386 g++-multilib

Install the required packages in a Fedora host system with:

.. code-block:: bash

   $ sudo yum groupinstall "Development Tools"
   $ sudo yum install git make gcc glib-devel.i686 \
     glib2-devel.i686 g++ libc6-dev-i386 g++-multilib \
     glibc-static libstdc++-static

.. _yocto_sdk:

Installing the Zephyr Software Development Kit
==============================================

Zephyr's :abbr:`SDK (Software Development Kit)` contains all
necessary tools and cross-compilers needed to build the kernel on all supported
architectures.
Additionally, it includes host tools such as a custom QEMU and a host compiler
for building host tools if necessary. With this SDK, there is no need to build
any cross compilers or emulation environments. The SDK supports the following
architectures:

* :abbr:`IA-32 (Intel Architecture 32 bits)`

* :abbr:`ARM (Advanced RISC Machines)`

* :abbr:`ARC (Argonaut RISC Core)`

Follow these steps to install the SDK on your host system.

#. Download the SDK self-extractable binary from:

   https://zephyr-download.01.org/zephyr-sdk/zephyr-sdk-0.6-i686-setup.run

    .. code-block:: bash

       $ wget --user=USERNAME --ask-password \
       https://zephyr-download.01.org/zephyr-sdk/zephyr-sdk-0.6-i686-setup.run

#. Run the installation binary, type:

    .. code-block:: bash

       $ chmod +x zephyr-sdk-0.6-i686-setup.run

       $ sudo ./zephyr-sdk-0.6-i686-setup.run


#. Follow the installation instructions on the screen. The
   toolchain's default installation location is :file:`/opt/zephyr-sdk/0.6`.

    .. code-block:: bash

       Verifying archive integrity... All good.

       Uncompressing SDK for Zephyr  100%

       Enter target directory for SDK (default: /opt/zephyr-sdk/0.6):

#. Enter a new location or hit :kbd:`Return` to accept default.

    .. code-block:: bash

       Installing SDK to /opt/zephyr-sdk/0.6

       Creating directory /opt/zephyr-sdk/0.6

       Success

       [*] Installing x86 tools...

       [*] Installing arm tools...

       [*] Installing arc tools...

       ...

       [*] Installing additional host tools...

       Success installing SDK. SDK is ready to be used.

#. To use the Zephyr SDK, export the following environment variables and
   use the target location where SDK was installed, type:

    .. code-block:: bash

       $ export ZEPHYR_GCC_VARIANT=zephyr

       $ export YOCTO_SDK_INSTALL_DIR=/opt/zephyr-sdk/0.6

Installing a Custom QEMU for ARM Platforms
==========================================

The SDK comes with a QEMU binary suitable for running sample |codename|
applications. The steps below are needed only if you choose not to use the
provided binary and use a custom built binary instead.

If you require testing ARM builds, a local patch to the QEMU source
is needed. The patch corrects the issues with the locking interfaces
QEMU uses. If you are working with the x86 builds of the kernel,
install QEMU from your systems default package manager.

Follow these steps to enable a customized build of QEMU:

#. Clone the QEMU repository:

   .. code-block:: bash

      $ git clone git://git.qemu-project.org/qemu.git

#. Checkout the v2.1 stable branch:

   .. code-block:: bash

      $ cd qemu

      $ git checkout stable-2.1

#. Apply our internal patch:

   .. code-block:: bash

      $ git am $ZEPHYR_BASE/scripts/0001-armv7m-support-basepri-primask-
      interrupt-locking.patch

#. Update the submodules as needed:

   .. code-block:: bash

      $ git submodule update --init pixman

      $ git submodule update --init dtc

#. Build QEMU v2.1:

   .. code-block:: bash

      $ ./configure && make

* You can also build QEMU to a private directory:

   .. code-block:: bash

      $ ./configure --prefix=$MY_PREFERED_INSTALL_LOCATION && make

* Install QEMU:

   .. code-block:: bash

      $ sudo make install

.. _setup_development_environment:

Setup a Local Development Environment
*************************************

The |project|'s source code is maintained using GIT and is served using
Gerrit.

Gerrit access requires some basic user setup. The following process shows
a simple walk-through to enable quick access to the Gerrit services.

.. _access_source:

Getting Access
==============

#. `Create`_ or `update`_ a `01.org`_ account.

#. Submit your your `01.org`_ account and corporate email address to
   |PM| hirally.santiago.rodriguez@intel.com.

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

If you already have a SSH key-pair, skip this section.

#. Create a key-pair in your Linux machine, type:

   .. code-block:: bash

      $ ssh-keygen -t rsa -C "John Doe john.doe@example.com"

   .. note:: This will ask you for a password to protect the private key as it
      generates a unique key. Please keep this password private, and DO
      NOT enter a blank password.


   The generated key-pair is found in:
   :file:`~/.ssh/id_rsa and ~/.ssh/id_rsa.pub`.

#. Add the private key in the :file:`id_rsa` file in your key ring:

   .. code-block:: bash

      $ ssh-add ~/.ssh/id_rsa


#. Add your public key :file:`id_rsa.pub` to the Gerrit account:

   a) Go to `access Gerrit`_.

   b) Click on your account name in the upper right corner.

   c) From the pop-up menu, select :guilabel:`Settings`.

   d) On the left side menu select, click on :guilabel:`SSH Public Keys`.

   e) Click Add key and paste the contents of your public key :file:`~/.id/id_rsa.pub`.

.. note:: To obtain the contents of your public key on a Linux machine:

   :command:`$ cat ~/.ssh/id_rsa.pub`

   The output is the contents of :file:`~/.id/id_rsa.pub`. Paste it into the
  'Add SSH key' window in Gerrit.

.. warning:: Potential Security Risk
   Do not copy your private key :file:`~/.ssh/id_rsa` Use only the public
   :file:`~/.id/id_rsa.pub`.

.. _checking_source_out:

Checking Out the Source Code
============================


#. Ensure that SSH has been set up properly. See `Configuring SSH to Use Gerrit`_ for details.

#. Clone the repository:

   .. code-block:: bash

      $ git clone ssh://01ORGUSERNAME@oic-review.01.org:29418/forto-collab

#. You have successfully checked out a copy of the source code to your local machine.
   Develop freely, issuing as many commits and rebases as needed.


#. Change to the main project directory:

    .. code-block:: bash

       $ cd forto-collab

#. Source the project environment file to setup project variables:

    .. code-block:: bash

       $ source zephyr-env.sh
