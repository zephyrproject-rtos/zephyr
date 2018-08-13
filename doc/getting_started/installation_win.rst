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

There are 3 different ways of developing for Zephyr on Microsoft Windows.
The first one is fully Windows native, whereas the 2 additional ones require
emulation layers that slow down build times and are not as optimal. All of
them are presented here for completeness, but unless you have a particular
requirement for a UNIX tool that is not available on Windows, we strongly
recommend you use the Windows Command Prompt for performance and minimal
dependency set.

Option 1: Windows Command Prompt
===================================================

The easiest way to install the dependencies natively on Microsoft Windows is
to use the :program:`Chocolatey` package manager (`Chocolatey website`_).
If you prefer to install those manually then simply download the required
packages from their respective websites.

.. note::
   There are multiple ``set`` statements in this tutorial. You can avoid
   typing them every time by placing them inside a ``.cmd`` file and
   running that every time you open a Command Prompt.

#. If you're behind a corporate firewall, you'll likely need to specify a
   proxy to get access to internet resources::

      set HTTP_PROXY=http://user:password@proxy.mycompany.com:1234
      set HTTPS_PROXY=http://user:password@proxy.mycompany.com:1234

#. Install :program:`Chocolatey` by following the instructions on the
   `Chocolatey install`_ website.

#. Open a Command Prompt (`cmd.exe`) as an **Administrator**.

#. Optionally disable global confirmation to avoid having to add `-y` to all
   commands:

   .. code-block:: console

      choco feature enable -n allowGlobalConfirmation

#. Install CMake:

   .. code-block:: console

      choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'

#. Install the rest of the tools:

   .. code-block:: console

      choco install git python ninja dtc-msys2 gperf doxygen.install

#. Close the Command Prompt window.

#. Open a Command Prompt (`cmd.exe`) as a **regular user**.

#. Clone a copy of the Zephyr source into your home directory using Git.

   .. code-block:: console

      cd %userprofile%
      git clone https://github.com/zephyrproject-rtos/zephyr.git

#. Install the required Python modules::

      cd %userprofile%\zephyr
      pip3 install -r scripts/requirements.txt

.. note::
      Although pip can install packages in the user's directory by means
      of the ``--user`` flag, this makes it harder for the Command Prompt
      to find the executables in Python modules installed by ``pip3``.

#. The build system should now be ready to work with any toolchain installed in
   your system. In the next step you'll find instructions for installing
   toolchains for building both x86 and ARM applications.

#. Install cross compiler toolchain:

   * For x86, install the 2017 Windows host ISSM toolchain from the Intel
     Developer Zone: `ISSM Toolchain`_. Use your web browser to
     download the toolchain's ``tar.gz`` file. You can then use 7-Zip or a
     similar tool to extract it into a destination folder.

     .. note::

        The ISSM toolset only supports development for Intel |reg| Quark |trade|
        Microcontrollers, for example, the Arduino 101 board.  (Check out the
        "Zephyr Development Environment
        Setup" in this `Getting Started on Arduino 101 with ISSM`_ document.)
        Additional setup is required to use the ISSM GUI for development.


   * For ARM, install GNU ARM Embedded from the ARM developer website:
     `GNU ARM Embedded`_ (install to :file:`c:\\gnuarmemb`).

#. Within the Command Prompt, set up environment variables for the installed
   tools and for the Zephyr environment:

   For x86:

   .. code-block:: console

      set ZEPHYR_TOOLCHAIN_VARIANT=issm
      set ISSM_INSTALLATION_PATH=c:\issm0-toolchain-windows-2017-01-25

   Use the path where you extracted the ISSM toolchain.

   For ARM:

   .. code-block:: console

      set ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
      set GNUARMEMB_TOOLCHAIN_PATH=c:\gnuarmemb

   To use the same toolchain in new sessions in the future you can set the
   variables in the file :file:`%userprofile%\\zephyrrc.cmd`.

   And for either, run the :file:`zephyr-env.cmd` file in order to set the
   :makevar:`ZEPHYR_BASE` environment variable:

   .. code-block:: console

      zephyr-env.cmd

.. note:: In previous releases of Zephyr, the ``ZEPHYR_TOOLCHAIN_VARIANT``
          variable was called ``ZEPHYR_GCC_VARIANT``.

#. Finally, you can try building the :ref:`hello_world` sample to check things
   out.

   To build for the Intel |reg| Quark |trade| (x86-based) Arduino 101:

   .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :host-os: win
     :generator: ninja
     :board: arduino_101
     :goals: build

   To build for the ARM-based Nordic nRF52 Development Kit:

   .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :host-os: win
     :generator: ninja
     :board: nrf52_pca10040
     :goals: build

This should check that all the tools and toolchain are set up correctly for
your own Zephyr development.

Option 2: MSYS2
===============

Alternatively, one can set up the Zephyr development environment with
MSYS2, a modern UNIX environment for Windows. Follow the steps below
to set it up:

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

      export http_proxy=http://proxy.mycompany.com:123
      export https_proxy=$http_proxy

#. Update MSYS2's packages and install the dependencies required to build
   Zephyr (you may need to restart the MSYS2 shell):

   .. code-block:: console

      pacman -Syu
      pacman -S git cmake make gcc dtc diffutils ncurses-devel python3 gperf

#. Compile :program:`Ninja` from source (Ninja is not available as
   an MSYS2 package) and install it:

   .. code-block:: console

      git clone git://github.com/ninja-build/ninja.git && cd ninja
      git checkout release
      ./configure.py --bootstrap
      cp ninja.exe /usr/bin/

#. From within the MSYS2 MSYS Shell, clone a copy of the Zephyr source
   into your home directory using Git.  (Some Zephyr tools require
   Unix-style line endings, so we'll configure Git for this repo to
   not do the automatic Unix/Windows line ending conversion (using
   ``--config core.autocrlf=false``).

   .. code-block:: console

      cd ~
      git clone --config core.autocrlf=false https://github.com/zephyrproject-rtos/zephyr.git

#. Install pip and the required Python modules::

      curl -O 'https://bootstrap.pypa.io/get-pip.py'
      ./get-pip.py
      rm get-pip.py
      cd ~/zephyr   # or to the folder where you cloned the zephyr repo
      pip install --user -r scripts/requirements.txt

#. The build system should now be ready to work with any toolchain installed in
   your system. In the next step you'll find instructions for installing
   toolchains for building both x86 and ARM applications.

#. Install cross compiler toolchain:

   * For x86, install the 2017 Windows host ISSM toolchain from the Intel
     Developer Zone: `ISSM Toolchain`_. Use your web browser to
     download the toolchain's ``tar.gz`` file.

     You'll need the tar application to unpack this file. In an ``MSYS2 MSYS``
     console, install ``tar`` and use it to extract the toolchain archive::

        pacman -S tar
        tar -zxvf /c/Users/myusername/Downloads/issm-toolchain-windows-2017-01-15.tar.gz -C /c

     substituting the .tar.gz path name with the one you downloaded.

     .. note::

        The ISSM toolset only supports development for Intel |reg| Quark |trade|
        Microcontrollers, for example, the Arduino 101 board.  (Check out the
        "Zephyr Development Environment
        Setup" in this `Getting Started on Arduino 101 with ISSM`_ document.)
        Additional setup is required to use the ISSM GUI for development.


   * For ARM, install GNU ARM Embedded from the ARM developer website:
     `GNU ARM Embedded`_ (install to :file:`c:\\gnuarmemb`).

#. Within the MSYS console, set up environment variables for the installed
   tools and for the Zephyr environment (using the provided shell script):

   For x86:

   .. code-block:: console

      export ZEPHYR_TOOLCHAIN_VARIANT=issm
      export ISSM_INSTALLATION_PATH=/c/issm0-toolchain-windows-2017-01-25

   Use the path where you extracted the ISSM toolchain.

   For ARM:

   .. code-block:: console

      export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
      export GNUARMEMB_TOOLCHAIN_PATH=/c/gnuarmemb

   And for either, run the provided script to set up zephyr project specific
   variables:

   .. code-block:: console

      unset ZEPHYR_SDK_INSTALL_DIR
      cd <zephyr git clone location>
      source zephyr-env.sh

#. Finally, you can try building the :ref:`hello_world` sample to check things
   out.

To build for the Intel |reg| Quark |trade| (x86-based) Arduino 101:

.. zephyr-app-commands::
  :zephyr-app: samples/hello_world
  :board: arduino_101
  :host-os: win
  :goals: build

To build for the ARM-based Nordic nRF52 Development Kit:

.. zephyr-app-commands::
  :zephyr-app: samples/hello_world
  :board: nrf52_pca10040
  :host-os: win
  :goals: build

This should check that all the tools and toolchain are set up correctly for
your own Zephyr development.

Option 3: Windows 10 WSL (Windows Subsystem for Linux)
======================================================

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
.. _Chocolatey website: https://chocolatey.org/
.. _Chocolatey install: https://chocolatey.org/install
.. _MSYS2 website: http://www.msys2.org/
.. _ISSM Toolchain: https://software.intel.com/en-us/articles/issm-toolchain-only-download
.. _Getting Started on Arduino 101 with ISSM: https://software.intel.com/en-us/articles/getting-started-arduino-101genuino-101-with-intel-system-studio-for-microcontrollers
.. _WSL Installation: https://msdn.microsoft.com/en-us/commandline/wsl/install_guide
