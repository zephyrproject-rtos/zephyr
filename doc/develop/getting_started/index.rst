.. _getting_started:

Getting Started Guide
#####################

Follow this guide to:

- Set up a command-line Zephyr development environment on Ubuntu, macOS, or
  Windows (instructions for other Linux distributions are discussed in
  :ref:`installation_linux`)
- Get the source code
- Build, flash, and run a sample application

.. _host_setup:

Select and Update OS
********************

Click the operating system you are using.

.. tabs::

   .. group-tab:: Ubuntu

      This guide covers Ubuntu version 20.04 LTS and later.

      .. code-block:: bash

         sudo apt update
         sudo apt upgrade

   .. group-tab:: macOS

      On macOS Mojave or later, select *System Preferences* >
      *Software Update*. Click *Update Now* if necessary.

      On other versions, see `this Apple support topic
      <https://support.apple.com/en-us/HT201541>`_.

   .. group-tab:: Windows

      Select *Start* > *Settings* > *Update & Security* > *Windows Update*.
      Click *Check for updates* and install any that are available.

.. _install-required-tools:

Install dependencies
********************

Next, you'll install some host dependencies using your package manager.

The current minimum required version for the main dependencies are:

.. list-table::
   :header-rows: 1

   * - Tool
     - Min. Version

   * - `CMake <https://cmake.org/>`_
     - 3.20.5

   * - `Python <https://www.python.org/>`_
     - 3.10

   * - `Devicetree compiler <https://www.devicetree.org/>`_
     - 1.4.6

.. tabs::

   .. group-tab:: Ubuntu

      .. _install_dependencies_ubuntu:

      #. If using an Ubuntu version older than 22.04, it is necessary to add extra
         repositories to meet the minimum required versions for the main
         dependencies listed above. In that case, download, inspect and execute
         the Kitware archive script to add the Kitware APT repository to your
         sources list.
         A detailed explanation of ``kitware-archive.sh`` can be found here
         `kitware third-party apt repository <https://apt.kitware.com/>`_:

         .. code-block:: bash

            wget https://apt.kitware.com/kitware-archive.sh
            sudo bash kitware-archive.sh

      #. Use ``apt`` to install the required dependencies:

         .. code-block:: bash

            sudo apt install --no-install-recommends git cmake ninja-build gperf \
              ccache dfu-util device-tree-compiler wget \
              python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
              make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

      #. Verify the versions of the main dependencies installed on your system by entering:

         .. code-block:: bash

            cmake --version
            python3 --version
            dtc --version

         Check those against the versions in the table in the beginning of this section.
         Refer to the :ref:`installation_linux` page for additional information on updating
         the dependencies manually.

   .. group-tab:: macOS

      .. _install_dependencies_macos:

      #. Install `Homebrew <https://brew.sh/>`_:

         .. code-block:: bash

            /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

      #. After the Homebrew installation script completes, follow the on-screen
         instructions to add the Homebrew installation to the path.

         * On macOS running on Apple Silicon, this is achieved with:

           .. code-block:: bash

              (echo; echo 'eval "$(/opt/homebrew/bin/brew shellenv)"') >> ~/.zprofile
              source ~/.zprofile

         * On macOS running on Intel, use the command for Apple Silicon, but replace ``/opt/homebrew/`` with ``/usr/local/``.

      #. Use ``brew`` to install the required dependencies:

         .. code-block:: bash

            brew install cmake ninja gperf python3 python-tk ccache qemu dtc libmagic wget openocd

      #. Add the Homebrew Python folder to the path, in order to be able to
         execute ``python`` and ``pip`` as well ``python3`` and ``pip3``.

           .. code-block:: bash

              (echo; echo 'export PATH="'$(brew --prefix)'/opt/python/libexec/bin:$PATH"') >> ~/.zprofile
              source ~/.zprofile

   .. group-tab:: Windows

      .. note::

         Due to issues finding executables, the Zephyr Project doesn't
         currently support application flashing using the `Windows Subsystem
         for Linux (WSL)
         <https://msdn.microsoft.com/en-us/commandline/wsl/install_guide>`_
         (WSL).

         Therefore, we don't recommend using WSL when getting started.

      These instructions must be run in a ``cmd.exe`` command prompt terminal window.
      In modern version of Windows (10 and later) it is recommended to install the Windows Terminal
      application from the Microsoft Store. The required commands differ on PowerShell.

      These instructions rely on `Chocolatey`_. If Chocolatey isn't an option,
      you can install dependencies from their respective websites and ensure
      the command line tools are on your :envvar:`PATH` :ref:`environment
      variable <env_vars>`.

      |p|

      .. _install_dependencies_windows:

      #. `Install chocolatey`_.

      #. Open a ``cmd.exe`` terminal window as **Administrator**. To do so, press the Windows key,
         type ``cmd.exe``, right-click the :guilabel:`Command Prompt` search result, and choose
         :guilabel:`Run as Administrator`.

      #. Disable global confirmation to avoid having to confirm the
         installation of individual programs:

         .. code-block:: bat

            choco feature enable -n allowGlobalConfirmation

      #. Use ``choco`` to install the required dependencies:

         .. code-block:: bat

            choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'
            choco install ninja gperf python311 git dtc-msys2 wget 7zip

         .. warning::

            As of November 2023, Python 3.12 is not recommended for Zephyr development on Windows,
            as some required Python dependencies may be difficult to install.

      #. Close the terminal window.

.. _Chocolatey: https://chocolatey.org/
.. _Install chocolatey: https://chocolatey.org/install

.. _get_the_code:
.. _clone-zephyr:
.. _install_py_requirements:
.. _gs_python_deps:

Get Zephyr and install Python dependencies
******************************************

Next, clone Zephyr and its :ref:`modules <modules>` into a new :ref:`west
<west>` workspace named :file:`zephyrproject`. You'll also install Zephyr's
additional Python dependencies in a `Python virtual environment`_.

.. _Python virtual environment: https://docs.python.org/3/library/venv.html

.. tabs::

   .. group-tab:: Ubuntu

      #. Use ``apt`` to install Python ``venv`` package:

         .. code-block:: bash

            sudo apt install python3-venv

      #. Create a new virtual environment:

         .. code-block:: bash

            python3 -m venv ~/zephyrproject/.venv

      #. Activate the virtual environment:

         .. code-block:: bash

            source ~/zephyrproject/.venv/bin/activate

         Once activated your shell will be prefixed with ``(.venv)``. The
         virtual environment can be deactivated at any time by running
         ``deactivate``.

         .. note::

            Remember to activate the virtual environment every time you
            start working.

      #. Install west:

         .. code-block:: bash

            pip install west

      #. Get the Zephyr source code:

         .. code-block:: bash

           west init ~/zephyrproject
           cd ~/zephyrproject
           west update

      #. Export a :ref:`Zephyr CMake package <cmake_pkg>`. This allows CMake to
         automatically load boilerplate code required for building Zephyr
         applications.

         .. code-block:: bash

            west zephyr-export

      #. Zephyr's ``scripts/requirements.txt`` file declares additional Python
         dependencies. Install them with ``pip``.

         .. code-block:: bash

            pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt

   .. group-tab:: macOS

      #. Create a new virtual environment:

         .. code-block:: bash

            python3 -m venv ~/zephyrproject/.venv

      #. Activate the virtual environment:

         .. code-block:: bash

            source ~/zephyrproject/.venv/bin/activate

         Once activated your shell will be prefixed with ``(.venv)``. The
         virtual environment can be deactivated at any time by running
         ``deactivate``.

         .. note::

            Remember to activate the virtual environment every time you
            start working.

      #. Install west:

         .. code-block:: bash

            pip install west

      #. Get the Zephyr source code:

         .. code-block:: bash

            west init ~/zephyrproject
            cd ~/zephyrproject
            west update

      #. Export a :ref:`Zephyr CMake package <cmake_pkg>`. This allows CMake to
         automatically load boilerplate code required for building Zephyr
         applications.

         .. code-block:: bash

            west zephyr-export

      #. Zephyr's ``scripts/requirements.txt`` file declares additional Python
         dependencies. Install them with ``pip``.

         .. code-block:: bash

            pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt

   .. group-tab:: Windows

      #. Open a ``cmd.exe`` terminal window **as a regular user**

      #. Create a new virtual environment:

         .. code-block:: bat

            cd %HOMEPATH%
            python -m venv zephyrproject\.venv

      #. Activate the virtual environment:

         .. code-block:: bat

            zephyrproject\.venv\Scripts\activate.bat

         Once activated your shell will be prefixed with ``(.venv)``. The
         virtual environment can be deactivated at any time by running
         ``deactivate``.

         .. note::

            Remember to activate the virtual environment every time you
            start working.

      #. Install west:

         .. code-block:: bat

            pip install west

      #. Get the Zephyr source code:

         .. code-block:: bat

            west init zephyrproject
            cd zephyrproject
            west update

      #. Export a :ref:`Zephyr CMake package <cmake_pkg>`. This allows CMake to
         automatically load boilerplate code required for building Zephyr
         applications.

         .. code-block:: bat

            west zephyr-export

      #. Zephyr's ``scripts\requirements.txt`` file declares additional Python
         dependencies. Install them with ``pip``.

         .. code-block:: bat

            pip install -r %HOMEPATH%\zephyrproject\zephyr\scripts\requirements.txt

Install the Zephyr SDK
**********************

The :ref:`Zephyr Software Development Kit (SDK) <toolchain_zephyr_sdk>`
contains toolchains for each of Zephyr's supported architectures, which
include a compiler, assembler, linker and other programs required to build
Zephyr applications.

For Linux, it also contains additional host tools, such as custom QEMU and OpenOCD builds
that are used to emulate, flash and debug Zephyr applications.


.. tabs::

   .. group-tab:: Ubuntu

      Install the Zephyr SDK using the ``west sdk install``.

         .. code-block:: bash

            cd ~/zephyrproject/zephyr
            west sdk install

      .. tip::

          Using the command options, you can specify the SDK installation destination
          and which architecture of toolchains to install.
          See ``west sdk install --help`` for details.

   .. group-tab:: macOS

      Install the Zephyr SDK using the ``west sdk install``.

         .. code-block:: bash

            cd ~/zephyrproject/zephyr
            west sdk install

      .. tip::

          Using the command options, you can specify the SDK installation destination
          and which architecture of toolchains to install.
          See ``west sdk install --help`` for details.

   .. group-tab:: Windows

      Install the Zephyr SDK using the ``west sdk install``.

         .. code-block:: bat

            cd %HOMEPATH%\zephyrproject\zephyr
            west sdk install

      .. tip::

          Using the command options, you can specify the SDK installation destination
          and which architecture of toolchains to install.
          See ``west sdk install --help`` for details.

.. note::

    If you want to install Zephyr SDK without using the ``west sdk`` command,
    please see :ref:`toolchain_zephyr_sdk_install`.

.. _getting_started_run_sample:

Build the Blinky Sample
***********************

.. note::

   :zephyr:code-sample:`blinky` is compatible with most, but not all, :ref:`boards`. If your board
   does not meet Blinky's :ref:`blinky-sample-requirements`, then
   :zephyr:code-sample:`hello_world` is a good alternative.

   If you are unsure what name west uses for your board, ``west boards``
   can be used to obtain a list of all boards Zephyr supports.

Build the :zephyr:code-sample:`blinky` with :ref:`west build <west-building>`, changing
``<your-board-name>`` appropriately for your board:

.. tabs::

   .. group-tab:: Ubuntu

      .. code-block:: bash

         cd ~/zephyrproject/zephyr
         west build -p always -b <your-board-name> samples/basic/blinky

   .. group-tab:: macOS

      .. code-block:: bash

         cd ~/zephyrproject/zephyr
         west build -p always -b <your-board-name> samples/basic/blinky

   .. group-tab:: Windows

      .. code-block:: bat

         cd %HOMEPATH%\zephyrproject\zephyr
         west build -p always -b <your-board-name> samples\basic\blinky

The ``-p always`` option forces a pristine build, and is recommended for new
users. Users may also use the ``-p auto`` option, which will use
heuristics to determine if a pristine build is required, such as when building
another sample.

.. note::

   A board may contain one or multiple SoCs, Also, each SoC may contain one or
   more CPU clusters.
   When building for such boards it is necessary to specify the SoC or CPU
   cluster for which the sample must be built.
   For example to build :zephyr:code-sample:`blinky` for the ``cpuapp`` core on
   the :ref:`nRF5340DK <nrf5340dk_nrf5340>` the board must be provided as:
   ``nrf5340dk/nrf5340/cpuapp``. See also :ref:`board_terminology` for more
   details.

Flash the Sample
****************

Connect your board, usually via USB, and turn it on if there's a power switch.
If in doubt about what to do, check your board's page in :ref:`boards`.

Then flash the sample using :ref:`west flash <west-flashing>`:

.. code-block:: shell

   west flash

You may need to install additional :ref:`host tools <flash-debug-host-tools>`
required by your board. The ``west flash`` command will print an error if any
required dependencies are missing.

If you're using blinky, the LED will start to blink as shown in this figure:

.. figure:: img/ReelBoard-Blinky.png
   :width: 400px
   :name: reelboard-blinky

   Phytec :ref:`reel_board <reel_board>` running blinky

Next Steps
**********

Here are some next steps for exploring Zephyr:

* Try other :zephyr:code-sample-category:`samples`
* Learn about :ref:`application` and the :ref:`west <west>` tool
* Find out about west's :ref:`flashing and debugging <west-build-flash-debug>`
  features, or more about :ref:`flashing_and_debugging` in general
* Check out :ref:`beyond-GSG` for additional setup alternatives and ideas
* Discover :ref:`project-resources` for getting help from the Zephyr
  community

.. _troubleshooting_installation:

Troubleshooting Installation
****************************

Here are some tips for fixing some issues related to the installation process.

.. _toolchain_zephyr_sdk_update:

Double Check the Zephyr SDK Variables When Updating
===================================================

When updating Zephyr SDK, check whether the :envvar:`ZEPHYR_TOOLCHAIN_VARIANT`
or :envvar:`ZEPHYR_SDK_INSTALL_DIR` environment variables are already set.
See :ref:`gs_toolchain_update` for more information.

For more information about these environment variables in Zephyr, see :ref:`env_vars_important`.

.. _help:

Asking for Help
***************

You can ask for help on a mailing list or on Discord. Please send bug reports and
feature requests to GitHub.

* **Mailing Lists**: users@lists.zephyrproject.org is usually the right list to
  ask for help. `Search archives and sign up here`_.
* **Discord**: You can join with this `Discord invite`_.
* **GitHub**: Use `GitHub issues`_ for bugs and feature requests.

How to Ask
==========

.. important::

   Please search this documentation and the mailing list archives first. Your
   question may have an answer there.

Don't just say "this isn't working" or ask "is this working?". Include as much
detail as you can about:

#. What you want to do
#. What you tried (commands you typed, etc.)
#. What happened (output of each command, etc.)

Use Copy/Paste
==============

Please **copy/paste text** instead of taking a picture or a screenshot of it.
Text includes source code, terminal commands, and their output.

Doing this makes it easier for people to help you, and also helps other users
search the archives. Unnecessary screenshots exclude vision impaired
developers; some are major Zephyr contributors. `Accessibility`_ has been
recognized as a basic human right by the United Nations.

When copy/pasting more than 5 lines of computer text into Discord or Github,
create a snippet using three backticks to delimit the snippet.

.. _Search archives and sign up here: https://lists.zephyrproject.org/g/users
.. _Discord invite: https://chat.zephyrproject.org
.. _GitHub issues: https://github.com/zephyrproject-rtos/zephyr/issues
.. _Accessibility: https://www.w3.org/standards/webdesign/accessibility
