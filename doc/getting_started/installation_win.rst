.. _installing_zephyr_win:

Install Windows Host Dependencies
#################################

.. important::

   Go back to the main :ref:`getting_started` when you're done here.

This guide was tested on Windows versions 7, 8.1, and 10.

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

There are 2 different ways of developing for Zephyr on Microsoft Windows:

#. :ref:`windows_install_native` (recommended)
#. :ref:`windows_install_wsl`

The first option is fully Windows native; the other requires emulation layers
that may result in slower build times.

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
   Chocolatey will properly set up your environment so tools can be
   found on your PATH.  If you install programs manually, be sure to
   verify the programs can also be found on your PATH.  For example,
   you may need to add ``C:\\Program Files\Git\bin`` after manually
   installing Git for Windows.

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

.. warning::
      Windows 10 version 1803 has an issue that will cause CMake to not work
      properly and is fixed in version 1809 (and later).
      More information can be found in `Zephyr Issue 10420`_

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

.. _Chocolatey: https://chocolatey.org/
.. _Chocolatey install: https://chocolatey.org/install
.. _Install the Windows Subsystem for Linux (WSL): https://msdn.microsoft.com/en-us/commandline/wsl/install_guide
.. _Zephyr Issue 10420: https://github.com/zephyrproject-rtos/zephyr/issues/10420
