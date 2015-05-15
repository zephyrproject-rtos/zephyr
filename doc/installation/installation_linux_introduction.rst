Introduction
############

Thank you for choosing Tiny Mountain. This release represents a snapshot
during the process of creating an open source project. Please work
closely with the project team to understand future direction and the
current implementation.

Tiny Mountain provides operating system capabilities targeted at very
small hardware configurations, while still offering a rich feature set
including cooperative multitasking and inter-task communication. Tiny
Mountain consists of both a nanokernel, for extremely memory-limited
devices, and a microkernel, for slightly larger devices. By definition,
all nanokernel capabilities are also available to a microkernel
configuration. Below you will see instructions for how to build and run
a Hello World program in the nanokernel.


General Information
*******************

These installation procedures were tested on a Linux host running Ubuntu
14.04 LTS 64 bit.

Only use these procedures to create a new Tiny Mountain development
environment. Do not use them to update an existing environment. They do
not account for file hierarchy changes that may occur from release to
release.

Perform the steps in the installation procedures in the order they
appear.

Building Tiny Mountain from Scratch
===================================

This task list shows the steps required to build a Tiny Mountain example
starting with a clean system.

#. Installing an operating system.
#. Configuring network and proxies.
#. Updating all the packages of your operating system.
#. Installing all general development requirements.
#. Cloning the Tiny Mountain repository.
#. Installing the appropriate toolchain.
#. Creating the needed build tools.
#. Building a Tiny Mountain example.

Installing the Operating System
-------------------------------

The steps needed for installing the operating system of the host system
are beyond the scope of this document. Please refer to the
documentation of your operating system of choice.

Configuring Network and Proxies
-------------------------------

The installation of Tiny Mountain requires the use of git, ssh, wget,
curl, and apt-get. The process for configuring a development system to
access through a firewall is beyond the scope of this document.

Before you continue ensure that your development system can use the
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