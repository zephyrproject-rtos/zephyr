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

Using MSYS2
===========

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

        If you need to inherit the existing Windows environment variables into
        MSYS2 you will need to create a **Windows** environment variable like so::
        ``MSYS2_PATH_TYPE=inherit``.

   .. note::
        There are multiple ``export`` statements in this tutorial. You can avoid
        typing them every time by placing them at the bottom of your
        ``~/.bash_profile`` file.

#. If you're behind a corporate firewall, you'll likely need to specify a
   proxy to get access to internet resources::

      $ export http_proxy=http://proxy.mycompany.com:123
      $ export https_proxy=$http_proxy

#. Update MSYS2's packages and install the dependencies required to build
   Zephyr (you may need to restart the MSYS2 shell):

   .. code-block:: console

      $ pacman -Syu
      $ pacman -S git cmake make gcc dtc diffutils ncurses-devel python3 gperf

#. From within the MSYS2 MSYS Shell, clone a copy of the Zephyr source into
   your home directory using Git:

   .. code-block:: console

      $ cd ~
      $ git clone https://github.com/zephyrproject-rtos/zephyr.git

#. Install pip and the required Python modules::

      $ curl -O 'https://bootstrap.pypa.io/get-pip.py'
      $ ./get-pip.py
      $ rm get-pip.py
      $ cd ~/zephyr   # or to the folder where you cloned the zephyr repo
      $ pip install --user -r scripts/requirements.txt

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

#. Within the MSYS console, set up environment variables for the installed
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
      $ cd <zephyr git clone location>
      $ source zephyr-env.sh

#. Within the MSYS console, build Kconfig in :file:`$ZEPHYR_BASE/build` and
    add it to path

   .. code-block:: console

      $ cd $ZEPHYR_BASE
      $ mkdir build && cd build
      $ cmake $ZEPHYR_BASE/scripts
      $ make
      $ echo "export PATH=$PWD/kconfig:\$PATH" >> $HOME/.zephyrrc
      $ source $ZEPHYR_BASE/zephyr-env.sh

    .. note::

        You only need to do this once after cloning the git repository.

#. Finally, you can try building the :ref:`hello_world` sample to check things
   out.

To build for the Intel |reg| Quark |trade| (x86-based) Arduino 101:

.. zephyr-app-commands::
  :zephyr-app: samples/hello_world
  :board: arduino_101
  :goals: build

To build for the ARM-based Nordic nRF52 Development Kit:

.. zephyr-app-commands::
  :zephyr-app: samples/hello_world
  :board: nrf52_pca10040
  :goals: build

This should check that all the tools and toolchain are set up correctly for
your own Zephyr development.

Using Windows 10 WSL (Windows Subsystem for Linux)
==================================================

If you are running a recent version of Windows 10 you can make use of the
built-in functionality to natively run Ubuntu binaries directly on a standard
command-prompt. This allows you to install the standard Zephyr SDK and build
for all supported architectures without the need for a Virtual Machine.

#. Install Windows Subsystem for Linux (WSL) following the instructions on the
   official Microsoft website: `WSL Installation`_

   .. note::
         For the Zephyr SDK to function properly you will need Windows 10
         build 15002 or greater. You can check which Windows 10 build you are
         running in the "About your PC" section of the System Settings.
         If you are running an older Windows 10 build you might need to install
         the Creator's Update.

#. Follow the instructions for Ubuntu detailed in the Zephyr Linux Getting
   Started Guide which can be found here: :ref:`installation_linux`

.. _GNU ARM Embedded: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
.. _MSYS2 website: http://www.msys2.org/
.. _ISSM Toolchain: https://software.intel.com/en-us/articles/issm-toolchain-only-download
.. _Getting Started on Arduino 101 with ISSM: https://software.intel.com/en-us/articles/getting-started-arduino-101genuino-101-with-intel-system-studio-for-microcontrollers
.. _WSL Installation: https://msdn.microsoft.com/en-us/commandline/wsl/install_guide
