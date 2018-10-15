.. _installing_zephyr_win:

Development Environment Setup on Windows
########################################

.. important::

   This section only describes OS-specific setup instructions; it is the first step in the
   complete Zephyr :ref:`getting_started`.

This section describes how to configure your development environment
to build Zephyr applications in a Microsoft Windows environment.

This guide was tested by building the Zephyr :ref:`hello_world` sample
application on Windows versions 7, 8.1, and 10.

Update Your Operating System
****************************

Before proceeding with the build, ensure that you are running your
Windows system with the latest updates installed.

.. _windows_requirements:

Install Requirements and Dependencies
*************************************

.. NOTE FOR DOCS AUTHORS: DO NOT PUT DOCUMENTATION BUILD DEPENDENCIES HERE.

   This section is for dependencies to build Zephyr binaries, *NOT* this
   documentation. If you need to add a dependency only required for building
   the docs, add it to doc/README.rst. (This change was made following the
   introduction of LaTeX->PDF support for the docs, as the texlive footprint is
   massive and not needed by users not building PDF documentation.)

There are 3 different ways of developing for Zephyr on Microsoft Windows:

#. :ref:`windows_install_native`
#. :ref:`windows_install_wsl`
#. :ref:`windows_install_msys2`

The first option is fully Windows native; the rest require emulation layers
that may result in slower build times. All three are included for completeness,
but unless you have a particular requirement for a UNIX tool that is not
available on Windows, we strongly recommend you use the Windows Command Prompt
option for performance and minimal dependency set. If you have a Unix tool
requirement, then we recommend trying the Windows Subsystem for Linux instead of
MSYS2.

.. _windows_install_native:

Option 1: Windows Command Prompt
================================

These instructions assume you are using the ``cmd.exe`` command prompt. Some of
the details, such as setting environment variables, may differ if you are using
PowerShell.

The easiest way to install the native Windows dependencies is to first install
`Chocolatey`_, a package manager for Windows.  If you prefer to install
dependencies manually, you can also download the required programs from their
respective websites.

.. note::
   There are multiple ``set`` statements in this tutorial. You can avoid
   typing them every time by placing them inside a ``.cmd`` file and
   running that every time you open a command prompt.

#. If you're behind a corporate firewall, you'll likely need to specify a
   proxy to get access to internet resources:

   .. code-block:: console

      set HTTP_PROXY=http://user:password@proxy.mycompany.com:1234
      set HTTPS_PROXY=http://user:password@proxy.mycompany.com:1234

#. Install :program:`Chocolatey` by following the instructions on the
   `Chocolatey install`_ page.

#. Open a command prompt (``cmd.exe``) as an **Administrator** (press the
   Windows key, type "cmd.exe" in the prompt, then right-click the result and
   choose "Run as Administrator").

#. Optionally disable global confirmation to avoid having to confirm
   installation of individual programs:

   .. code-block:: console

      choco feature enable -n allowGlobalConfirmation

#. Install CMake:

   .. code-block:: console

      choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'

#. Install the rest of the tools:

   .. code-block:: console

      choco install git python ninja dtc-msys2 gperf

#. Close the Administrator command prompt window.

.. NOTE FOR DOCS AUTHORS: as a reminder, do *NOT* put dependencies for building
   the documentation itself here.

.. _windows_install_wsl:

Option 2: Windows 10 WSL (Windows Subsystem for Linux)
======================================================

If you are running a recent version of Windows 10 you can make use of the
built-in functionality to natively run Ubuntu binaries directly on a standard
command-prompt. This allows you to use software such as the :ref:`Zephyr SDK
<zephyr_sdk>` without setting up a virtual machine.

#. `Install the Windows Subsystem for Linux (WSL)`_.

   .. note::
         For the Zephyr SDK to function properly you will need Windows 10
         build 15002 or greater. You can check which Windows 10 build you are
         running in the "About your PC" section of the System Settings.
         If you are running an older Windows 10 build you might need to install
         the Creator's Update.

#. Follow the Ubuntu instructions in the :ref:`installation_linux` document.

.. NOTE FOR DOCS AUTHORS: as a reminder, do *NOT* put dependencies for building
   the documentation itself here.

.. _windows_install_msys2:

Option 3: MSYS2
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
      pacman -S git cmake make gcc dtc diffutils ncurses-devel python3 gperf tar

#. Compile :program:`Ninja` from source (Ninja is not available as
   an MSYS2 package) and install it:

   .. code-block:: console

      git clone git://github.com/ninja-build/ninja.git && cd ninja
      git checkout release
      ./configure.py --bootstrap
      cp ninja.exe /usr/bin/

#. Install pip and the required Python modules::

      curl -O 'https://bootstrap.pypa.io/get-pip.py'
      ./get-pip.py
      rm get-pip.py

You're now almost ready to continue with the rest of the getting started guide.

Since you're using MSYS2, when you're cloning Zephyr in the next step of the
guide, use this command line instead (i.e. add the ``--config
core.autocrlf=false`` option).

   .. code-block:: console

      git clone --config core.autocrlf=false https://github.com/zephyrproject-rtos/zephyr

Furthermore, when you start installing Python dependencies, you'll want to add
the ``--user`` option as is recommended on Linux.

.. NOTE FOR DOCS AUTHORS: as a reminder, do *NOT* put dependencies for building
   the documentation itself here.

.. _Chocolatey: https://chocolatey.org/
.. _Chocolatey install: https://chocolatey.org/install
.. _MSYS2 website: http://www.msys2.org/
.. _Install the Windows Subsystem for Linux (WSL): https://msdn.microsoft.com/en-us/commandline/wsl/install_guide
