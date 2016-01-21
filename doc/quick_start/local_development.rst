.. _setup_development_environment:

Setup a Local Development Environment
#####################################

.. _zephyr_sdk:

Installing the Zephyr Software Development Kit
==============================================

Zephyr's :abbr:`SDK (Software Development Kit)` contains all
necessary tools and cross-compilers needed to build the kernel on all supported
architectures.
Additionally, it includes host tools such as a custom QEMU and a host compiler
for building host tools if necessary. The SDK supports the following
architectures:

* :abbr:`IA-32 (Intel Architecture 32 bits)`

* :abbr:`ARM (Advanced RISC Machines)`

* :abbr:`ARC (Argonaut RISC Core)`

Follow these steps to install the SDK on your host system.

#. Download the SDK self-extractable binary from:

   https://zephyr-download.01.org/zephyr-sdk/zephyr-sdk-0.7.1-i686-setup.run

    .. code-block:: console

       $ wget --user=USERNAME --ask-password \
       https://zephyr-download.01.org/zephyr-sdk/zephyr-sdk-0.7.1-i686-setup.run

#. Run the installation binary, type:

    .. code-block:: console

       $ chmod +x zephyr-sdk-0.7.1-i686-setup.run

       $ sudo ./zephyr-sdk-0.7.1-i686-setup.run


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

       $ export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk/




The Zephyr Project's source code is maintained using Git and is served using
Gerrit.

Gerrit access requires some basic user setup. The following process shows
a simple walk-through to enable quick access to the Gerrit services.

.. _access_source:

Getting Access
==============

#. `Create`_ or `update`_ a `01.org`_ account.

#. Request access by contacting the Zephyr project team.

#. Once access is granted, `access Gerrit`_.

#. Log in using your 01.org account credentials.

.. _Create: https://01.org/user/register

.. _update: https://01.org/user/login

.. _access Gerrit: https://oic-review.01.org/gerrit/

.. _01.org: https://01.org/

Configuring SSH to Use Gerrit
=============================

Gerrit uses SSH to interact with your Git client. A SSH private key
needs to be generated on the development machine with a matching public
key on the Gerrit server.

If you already have a SSH key-pair, skip this section.

#. Create a key-pair in your Linux machine, type:

   .. code-block:: console

      $ ssh-keygen -t rsa -C "John Doe john.doe@example.com"

   .. note:: This will ask you for a password to protect the private key as it
      generates a unique key. Please keep this password private, and DO
      NOT enter a blank password.


   The generated key-pair is found in:
   :file:`~/.ssh/id_rsa and ~/.ssh/id_rsa.pub`.

#. Add the private key in the :file:`id_rsa` file in your key ring:

   .. code-block:: console

      $ ssh-add ~/.ssh/id_rsa


#. Add your public key :file:`id_rsa.pub` to the Gerrit account:

   a) Go to `access Gerrit`_.

   b) Click on your account name in the upper right corner.

   c) From the pop-up menu, select :guilabel:`Settings`.

   d) On the left side menu, click on :guilabel:`SSH Public Keys`.

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


#. Ensure that SSH has been set up properly. See `Configuring SSH to Use Gerrit`_
   for details.

#. Clone the repository:

   .. code-block:: console

      $ git clone ssh://01ORGUSERNAME@oic-review.01.org:29418/forto-collab zephyr-project

#. You have successfully checked out a copy of the source code to your local machine.

#. Change to the main project directory:

    .. code-block:: console

       $ cd zephyr-project

#. Source the project environment file to setup project variables:

    .. code-block:: console

       $ source zephyr-env.sh
