.. _installing_zephyr:

Development Environment Setup
#############################

This section describes how to build the kernel in a development system
and how to access the project source code.

.. _linux_development_system:

Installing the Host's Operating System
======================================

Building the project's software components including the kernel has been tested
on Ubuntu and  Fedora systems. Instructions for installing these OSes are beyond
the scope of this document.

Configuring Network and Proxies
===============================

Building the kernel requires the command-line tools of git, ssh, wget,
curl. Verify that each service can be run as both user and root and that access
to the Internet and is not impeded by a firewall.

Update Your Operating System
============================

Before proceeding with the build, ensure your OS is up to date. On Ubuntu:

.. code-block:: console

   $ sudo apt-get update

On Fedora:

.. code-block:: console

   $ sudo dnf update

.. _required_software:

Installing Requirements and Dependencies
========================================

Install the following with either apt-get or dnf.

.. note::
   Minor version updates of the listed required packages might also
   work.

Install the required packages in a Ubuntu host system with:

.. code-block:: console

   $ sudo apt-get install git make gcc gcc-multilib g++ libc6-dev-i386 \
     g++-multilib

Install the required packages in a Fedora host system with:

.. code-block:: console

   $ sudo dnf group install "Development Tools"
   $ sudo dnf install git make gcc glib-devel.i686 glib2-devel.i686 \
     glibc-static libstdc++-static


