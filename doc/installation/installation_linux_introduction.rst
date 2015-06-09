General Information
############

Only use these procedures to create a new |project| development
environment. Do not use them to update an existing environment. They do
not account for file hierarchy changes that may occur from release to
release.

Perform the steps in the installation procedures in the order they
appear.

Building Zephyr Applications from Scratch
==========================================

This task list shows the steps required to build a sample application
starting with a clean system.

#. Installing an operating system.
#. Configuring network and proxies.
#. Updating all the packages of your operating system.
#. Installing all general development requirements.
#. Cloning the repository.
#. Installing the appropriate toolchain.
#. Creating the needed build tools.
#. Building an example.

Installing the Operating System
-------------------------------

The steps needed for installing the operating system of the host system
are beyond the scope of this document. Please refer to the
documentation of your operating system of choice.  We recommend the use of
Ubuntu or Fedora.

Configuring Network and Proxies
-------------------------------

The installation process requires the use of git, ssh, wget,
curl, and apt-get. The installation and configuration of these services for
a development system is beyond the scope of this document.  Once installed,
verify that each service can access the Internet and is not impeded by a
firewall.

Before you continue, ensure that your development system can use the
above commands in both User and root configurations. Please refer to
the documentation of your operating system of choice.


Update Your Operating System
----------------------------

In Ubuntu enter:

.. code-block:: bash

   $ sudo apt-get update

In Fedora enter:

.. code-block:: bash

   $ sudo yum update
