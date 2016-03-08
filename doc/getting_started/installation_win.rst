.. _installing_zephyr_win:

Development Environment Setup on Windows
########################################

This section describes how to configure your development environment and
to build Zephyr applications in a Microsoft Windows environment.

This guide was tested by compiling and running the Zephyr's sample
applications on the following Windows version:

* Windows 8.1

Update Your Operating System
****************************

Before proceeding with the build, ensure that you are running your
Windows system with the latest updates installed.

.. _windows_requirements:

Installing Requirements and Dependencies
****************************************

To install the software components required to build Zephyr applications on
Windows, you will need to build or install a toolchain.

Install :program:`GIT`. Go to `GIT Download`_ to obtain the latest copy of
the software.

Install :program:`Python 2.7`. Go to `Python Download`_ to obtain the 2.7
version of the software.

Install :program:`MinGW`. MinGW is the minimalist GNU development environment
for native Windows applications. The Zephyr build system will execute on top of
this tool set.

To install :program:`MinGW`, visit the site `MinGW Home`_ and install the
following packages:

* mingw-developer-toolkit
* mingw32-base
* msys-base
* msys-binutils
* msys-w32api

In the :program:`MSYS console` enable MinGW tools in MSYS. Edit the
file :file:`/etc/fstab` and add the following lines:

.. code-block:: console

   #Win32_Path     Mount_Point
   c:/mingw             /mingw

Alternatively, you can copy the file :file:`fstab.sample` as :file:`fstab`
and confirm that the these lines are in the new :file:`fstab` file.

Configure Python's folder location in the environmental variable :envvar:`PATH`.

.. code-block:: console

   export PATH=$PATH:${PYTHON_PATH}

.. note:: The format of the path for this variable (PYTHON_PATH) must to be in
   the linux format. For example, :file:`C:\python27` would be written as
   :file:`/c/python27/`.

GNU Regex C library
===================

The Zephyr build process has a dependency with the GNU regex library.
Msys provides its own GNU library implementation that can be downloaded from the
MinGW and Msys official repository:`MinGW Repository`_.
Install the library from the Msys console interface and add the library to the
tools build proccess with the following commands:

.. code-block:: console

   mingw-get update
   mingw-get install msys-libregex-dev --all-related
   export HOST_LOADLIBES=-lregex

Toolchain Installation
======================

The build system should be able to work with any toolchain installed in your system.

For instance, the Zephyr build system was tested with the toolchain provided with
the ISSM 2016 (Intel System Studio for Microcontrollers) installation.

To install ISSM use the link provided to download from the Intel Developer Zone:
`ISSM 2016 Download`_ and install it into your system.

Finally, configure your environment variables for the ISSM 2016 toolchain.
For example, using the default installation path for ISSM:
:file:`/c/IntelSWTools/ISSM_2016.0.27/tools/compiler`

.. code-block:: console

    export ZEPHYR_GCC_VARIANT=iamcu
    export IAMCU_TOOLCHAIN_PATH=/c/IntelSWTools/ISSM_2016.0.27/tools/compiler

.. note:: The format of the location for the toolchain installation directory
   (e.g. :envvar:`IAMCU_TOOLCHAIN_PATH`) must be in the linux format. E.g.
   :file:`C:\toolchain` would be written as :file:`/c/toolchain/`.

.. _GIT Download: https://git-scm.com/download/win
.. _Python Download: https://www.python.org/downloads/
.. _MinGW Home: http://www.mingw.org/
.. _MinGW Repository: http:sourceforge.net/projects/mingw/files/
.. _ISSM 2016 Download: https://software.intel.com/en-us/intel-system-studio-microcontrollers
