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

The Zephyr development environment on Windows relies on MSYS2, a modern UNIX
environment for Windows. Follow the steps below to set it up:

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

        Make sure you start ``MSYS2 MSYS Shell``, not ``MSYS2 MinGW Shell``.

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
        $ git clone https://git.kernel.org/pub/scm/utils/dtc/dtc.git
        $ cd dtc
        $ make

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


.. _GNU ARM Embedded: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
.. _MSYS2 website: http://www.msys2.org/
.. _ISSM Toolchain: https://software.intel.com/en-us/articles/issm-toolchain-only-download
.. _Getting Started on Arduino 101 with ISSM: https://software.intel.com/en-us/articles/getting-started-arduino-101genuino-101-with-intel-system-studio-for-microcontrollers
