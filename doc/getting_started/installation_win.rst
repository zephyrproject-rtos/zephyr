.. _installing_zephyr_win:

Development Environment Setup on Windows
########################################

This section describes how to configure your development environment and
to build Zephyr applications in a Microsoft Windows environment.

This guide was tested by building the Zephyr :ref:`hello_world` sample
application on Windows versions 7, 8.1, and 10.

Update Your Operating System
****************************

Before proceeding with the build, ensure that you are running your
Windows system with the latest updates installed.

.. _windows_requirements:

Installing Requirements and Dependencies
****************************************

Using MSYS2 (recommended)
=========================

The simplest way to install a development environment on Windows is to use
MSYS2, a modern UNIX environment for Windows. Follow the steps below to set it
up:

#. Download and install :program:`MSYS2`. Download the appropriate (32 or
   64-bit) MSYS2 installer from the `MSYS2 website`_ and execute it. On the
   final installation screen, check the "Run MSYS2 now." box to start up an
   MSYS2 shell when installation is complete.  Follow the rest of the
   installation instructions on the MSYS2 website to update the package
   database and core system packages.  You may be advised to "terminate MSYS2
   without returning to shell and check for updates again".  If so, simply
   close the ``MSYS2 MSYS Shell`` desktop app and run it again to complete the update.)

#. Launch the ``MSYS2 MSYS Shell`` desktop app from your start menu (if it's not still open).

     .. note::
        There are multiple ``export`` statements in this tutorial. You can avoid
        typing them every time by placing them at the bottom of your
        ``~/.bash_profile`` file.

#. If you're behind a corporate firewall, you'll likely need to specify a
   proxy to get access to internet resources::

      $ export http_proxy=http://proxy.mycompany.com:123
      $ export https_proxy=$http_proxy

#. Install the dependencies required to build Zephyr:

   .. code-block:: console

      $ pacman -S git make gcc diffutils ncurses-devel python3

#. Install pip and the required Python modules::

      $ curl -O 'https://bootstrap.pypa.io/get-pip.py'
      $ ./get-pip.py
      $ rm get-pip.py

      $ pip install pyaml

#. Build the Device Tree Compiler (DTC)

   For the architectures and boards listed in the ``dts/`` folder of the Zephyr
   source tree, the DTC is required to be able to build Zephyr.
   To set up the DTC follow the instructions below:

   * Install the required build tools::

        $ pacman -S bison
        $ pacman -R flex
        $ pacman -U http://repo.msys2.org/msys/x86_64/flex-2.6.0-1-x86_64.pkg.tar.xz

   .. note::
        At this time we need to pin the ``flex`` version to an older one due
        to an issue with the latest available.

   * Clone and build the DTC::

        $ cd ~
        $ git clone https://github.com/carlescufi/dtc.git
        $ cd dtc
        $ make

   .. note::
        The repository containing the DTC itself is currently not the standard
        upstream one due to a required patch. This will be updated once the
        patch has been accepted.

   * Export the location of the DTC::

        $ export DTC=~/dtc/dtc

#. The build system should now be ready to work with any toolchain installed in
   your system. In the next step you'll find instructions for installing
   toolchains for building both x86 and ARM applications.

#. Install cross compiler toolchain:

   * For x86, install the 2017 Windows host ISSM toolchain from the Intel
     Developer Zone: `ISSM Toolchain`_. Use your web browser to
     download the toolchain's ``tar.gz`` file.

     You'll need the tar application to unpack this file. In an ``MSYS2 MSYS``
     console, install ``tar`` and use it to extract the toolchain archive::

        $ pacman -S tar
        $ tar -zxvf /c/Users/myusername/Downloads/issm-toolchain-windows-2017-01-15.tar.gz -C /c

     substituting the .tar.gz path name with the one you downloaded.

     .. note::

        The ISSM toolset only supports development for Intel |reg| Quark |trade|
        Microcontrollers, for example, the Arduino 101 board.  (Check out the
        "Zephyr Development Environment
        Setup" in this `Getting Started on Arduino 101 with ISSM`_ document.)
        Additional setup is required to use the ISSM GUI for development.


   * For ARM, install GNU ARM Embedded from the ARM developer website:
     `GNU ARM Embedded`_ (install to :file:`c:\\gccarmemb`).

#. From within the `MSYS2 MSYS Shell`, clone a copy of the Zephyr source into
   your home directory using Git:

   .. code-block:: console

      $ cd ~
      $ git clone https://github.com/zephyrproject-rtos/zephyr.git

#. Also within the MSYS console, set up environment variables for the installed
   tools and for the Zephyr environment (using the provided shell script):

   For x86:

   .. code-block:: console

      $ export ZEPHYR_GCC_VARIANT=issm
      $ export ISSM_INSTALLATION_PATH=/c/issm0-toolchain-windows-2017-01-25

   Use the path where you extracted the ISSM toolchain.

   For ARM:

   .. code-block:: console

      $ export ZEPHYR_GCC_VARIANT=gccarmemb
      $ export GCCARMEMB_TOOLCHAIN_PATH=/c/gccarmemb

   And for either, run the provided script to set up zephyr project specific
   variables:

   .. code-block:: console

      $ unset ZEPHYR_SDK_INSTALL_DIR
      $ source ~/zephyr/zephyr-env.sh

#. Finally, you can try building the :ref:`hello_world` sample to check things
   out.

   To build for the Intel |reg| Quark |trade| (x86-based) Arduino 101:

    .. code-block:: console

       $ cd $ZEPHYR_BASE/samples/hello_world
       $ make BOARD=arduino_101

   To build for the ARM-based Nordic nRF52 Development Kit:

    .. code-block:: console

       $ cd $ZEPHYR_BASE/samples/hello_world
       $ make BOARD=nrf52_pca10040


    This should check that all the tools and toolchain are set up correctly for
    your own Zephyr development.


Using MinGW (not recommended)
=============================

To install the software components required to build Zephyr applications on
Windows, you will need to build or install a toolchain:

1. Install :program:`GIT`. Go to `GIT Download`_ to obtain the latest copy of
   the software (2.12.0).  Install into the :file:`C:\\Git` folder and use the
   default configuration options for the rest.

2. Install :program:`Python 2.7`. Go to `Python Download`_ to obtain the
   software (version 2.7.13) and use the default installation options.

3. Install :program:`MinGW`. MinGW is the minimalist GNU development environment
   for native Windows applications. The Zephyr build system will execute on top
   of this tool set.  Visit the site `MinGW Home`_ and install the
   following packages with their installer `mingw-get-setup.exe` (you'll need
   to open the "All Packages" tab to enable installing the msys packages listed
   here):

   * mingw-developer-toolkit
   * mingw32-base
   * msys-base
   * msys-binutils
   * msys-console
   * msys-w32api

4. Launch the `MSYS console` from a cmd window. The installer does not create
   shortcuts for you so you'll need to run the script
   in :file:`C:\\MinGW\\msys\\1.0\\msys.bat.`

5. The Zephyr build process has a dependency on the Pthread and GNU regex
   libraries.  Msys provides its own GNU library implementation that can be
   downloaded from the MinGW and Msys official repository:
   `MinGW Repository`_ with the following commands:

   .. code-block:: console

      $ mingw-get update
      $ mingw-get install libpthread msys-libregex-dev --all-related


   When done, move libregex files (``libregex.a``, ``libregex.dll.a``,
   ``libregex.la``)
   from ``C:\Git\mingw32\msys\1.0\lib`` to ``C:\Git\mingw32\lib``

6. We need to edit :file:`/etc/fstab` to create an entry mapping from the Win32
   path ``c:/mingw`` to the mount point ``/mingw``
   The easiest way to do this is just copy the file :file:`fstab.sample` as
   :file:`fstab` and ``cat /etc/fstab`` to confirm that the mapping was added.


7. The build system should be able to work with any toolchain installed in your
   system. For instance, the Zephyr build system was tested using the mingw
   MSYS console (as described below) with the toolchain
   provided with the ISSM 2016 (Intel System Studio for Microcontrollers)
   installation.  Install ISSM toolchain from the Intel Developer Zone:
   `ISSM Toolchain`_

   .. note::

      The ISSM toolset only supports development for Intel |reg| Quark |trade|
      Microcontrollers, for example, the Arduino 101 board.  (Check out the
      "Zephyr Development Environment
      Setup" in this `Getting Started on Arduino 101 with ISSM`_ document.)
      Also, additional setup is required to use the ISSM GUI for
      development.

8. From within the MSYS console, clone a copy of the Zephyr source into your
   home directory using Git:

   .. code-block:: console

      $ cd ~
      $ git clone https://github.com/zephyrproject-rtos/zephyr.git

9. Also within the MSYS console, set up environment variables for installed
   tools and for the Zephyr environment (using the provided shell script):

   .. code-block:: console

      $ export PATH=$PATH:/c/python27/
      $ export MINGW_DIR=/c/mingw
      $ export ZEPHYR_GCC_VARIANT=issm
      $ export ISSM_INSTALLATION_PATH=C:/IntelSWTools/ISSM_2016.1.067
      $ unset ZEPHYR_SDK_INSTALL_DIR
      $ source ~/zephyr/zephyr-env.sh

10. Finally, you can try building the :ref:`hello_world` sample to check things
    out.  In this example, we'll build the hello_world sample for the Arduino
    101 board:

    .. code-block:: console

       $ cd $ZEPHYR_BASE/samples/hello_world
       $ make BOARD=arduino_101

    This should check that all the tools and toolchain are setup correctly for
    your own Zephyr development.


.. _GNU ARM Embedded: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
.. _MSYS2 website: http://www.msys2.org/
.. _GIT Download: https://git-scm.com/download/win
.. _Python Download: https://www.python.org/downloads/
.. _MinGW Home: http://www.mingw.org/
.. _MinGW Repository: http://sourceforge.net/projects/mingw/files/
.. _ISSM Toolchain: https://software.intel.com/en-us/articles/issm-toolchain-only-download
.. _Getting Started on Arduino 101 with ISSM: https://software.intel.com/en-us/articles/getting-started-arduino-101genuino-101-with-intel-system-studio-for-microcontrollers
