.. _installing_zephyr_mac:

Development Environment Setup on macOS
######################################

.. important::

   This section only describes OS-specific setup instructions; it is the first step in the
   complete Zephyr :ref:`getting_started`.

This section describes how to set up a Zephyr development environment on macOS.

These instructions have been tested on the following macOS versions:

* Mac OS X 10.11 (El Capitan)
* macOS Sierra 10.12

Update Your Operating System
****************************

Before proceeding with the build, ensure your OS is up to date.

.. _mac_requirements:

Install Requirements and Dependencies
*************************************

.. NOTE FOR DOCS AUTHORS: DO NOT PUT DOCUMENTATION BUILD DEPENDENCIES HERE.

   This section is for dependencies to build Zephyr binaries, *NOT* this
   documentation. If you need to add a dependency only required for building
   the docs, add it to doc/README.rst. (This change was made following the
   introduction of LaTeX->PDF support for the docs, as the texlive footprint is
   massive and not needed by users not building PDF documentation.)

.. note::

   Zephyr requires Python 3, while macOS only provides a Python 2
   installation. After following these instructions, the version of Python 2
   provided by macOS in ``/usr/bin/`` will sit alongside the Python 3
   installation from Homebrew in ``/usr/local/bin``.

First, install :program:`Homebrew` by following instructions on the `Homebrew
site`_. Homebrew is a free and open-source package management system that
simplifies the installation of software on macOS.  While installing Homebrew,
you may be prompted to install additional missing dependencies; please follow
any such instructions as well.

After Homebrew is successfully installed, install the following tools using
the ``brew`` command line tool in the Terminal application.

.. code-block:: console

   brew install cmake ninja gperf ccache dfu-util qemu dtc python3

.. _Homebrew site: https://brew.sh/
