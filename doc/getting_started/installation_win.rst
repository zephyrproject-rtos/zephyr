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
following packages with their installler `mingw-get-setup.exe`:

* mingw-developer-toolkit
* mingw32-base
* msys-base
* msys-binutils
* msys-console
* msys-w32api

Launch the `MSYS console`. The installer does not create shortcuts for you,
but the script to launch it is in :file:`C:\\MinGW\\msys\\1.0\\msys.bat.`.
We need the following line in :file:`/etc/fstab`:

.. code-block:: console

   #Win32_Path     Mount_Point
   c:/mingw             /mingw

The easiest way to do this is just copy the file :file:`fstab.sample` as
:file:`fstab` and confirm that the these lines are in the new
:file:`fstab` file.

.. code-block:: console

   $ cp /etc/fstab.sample /etc/fstab
   $ cat /etc/fstab

Configure Python's folder location in the environmental variable :envvar:`PATH`
and the installation path for MinGW.

.. note:: The format of the path for `PYTHON_PATH` must to be in the
   linux format. Default installation is in :file:`C:\\python27`,
   which would be written as :file:`/c/python27/`.

.. code-block:: console

   export PYTHON_PATH=/c/python27
   export PATH=$PATH:${PYTHON_PATH}
   export MINGW_DIR=/c/MinGW

Pthread library
===============

The Zephyr OS build process has a dependency on the Pthread library.
The required packages for Msys installation would normally provide it.
However, if a minimal installation fails to provide the Pthread library,
it can be installed with the following command:

.. code-block:: console

   mingw-get install libpthread

GNU Regex C library
===================

The Zephyr build process has a dependency with the GNU regex library.
Msys provides its own GNU library implementation that can be downloaded from the
MinGW and Msys official repository:`MinGW Repository`_.
Install the library from the Msys console interface with the following commands:

.. code-block:: console

   mingw-get update
   mingw-get install msys-libregex-dev --all-related

Update the following environment variables on your system to allow the C compiler
and linker to find the library and headers:

.. code-block:: console

   export LIBRARY_PATH=$LIBRARY_PATH:/c/mingw/msys/1.0/lib
   export C_INCLUDE_PATH=$C_INCLUDE_PATH:/c/mingw/msys/1.0/include

Toolchain Installation
======================

The build system should be able to work with any toolchain installed in your system.

For instance, the Zephyr build system was tested with the toolchain provided with
the ISSM 2016 (Intel System Studio for Microcontrollers) installation.

To install ISSM use the link provided to download from the Intel Developer Zone:
`ISSM 2016 Download`_ and install it into your system.

Finally, configure your environment variables for the ISSM 2016 toolchain.
For example, using the default installation path for ISSM:
:file:`C:/IntelSWTools/ISSM_2016`

.. code-block:: console

    export ZEPHYR_GCC_VARIANT=issm
    export ISSM_INSTALLATION_PATH=C:/IntelSWTools/ISSM_2016

.. note:: The format of the location for the ISSM installation directory
   (e.g. :envvar:`ISSM_INSTALLATION_PATH`) must be in the windows format.

.. _GIT Download: https://git-scm.com/download/win
.. _Python Download: https://www.python.org/downloads/
.. _MinGW Home: http://www.mingw.org/
.. _MinGW Repository: http:sourceforge.net/projects/mingw/files/
.. _ISSM 2016 Download: https://software.intel.com/en-us/intel-system-studio-microcontrollers
