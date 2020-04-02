.. _getting_started:

Getting Started Guide
#####################

Follow this guide to get a quick start with Zephyr development where
you'll:

- **Set up a command-line development environment** for Linux* (Ubuntu),
  macOS, or Windows, with required package manager, compiler, and
  build-system tools,
- **Get the sources**,
- **Build, flash, and run** a sample application on your target board.

\*Instructions for other Linux distributions are discussed in the
:ref:`advanced Linux setup document <installation_linux>`.

.. _host_setup:

.. rst-class:: numbered-step

Select and Update OS
********************

Zephyr development depends on an up-to-date host system and common build system
tools. First, make sure your development system OS is updated:

.. tabs::

   .. group-tab:: Ubuntu

      This guide covers Ubuntu version 18.04 LTS and later. See
      :ref:`installation_linux` for information about other Linux
      distributions and older versions of Ubuntu.

      Use these commands to bring your Ubuntu system up to date:

      .. code-block:: bash

         sudo apt update
         sudo apt upgrade

   .. group-tab:: macOS

      On macOS Mojave or later, you can manually check for updates by choosing
      System Preferences from the Apple menu, then clicking Software Update (and
      click Update Now if there are). For other macOS versions, see the
      `Update macOS topic in Apple support
      <https://support.apple.com/en-us/HT201541>`_.

   .. group-tab:: Windows

      On Windows, you can manually check for updates by selecting Start  > Settings  >
      Update & Security  > Windows Update, and then select Check for updates.
      If updates are available, install them.

.. _install-required-tools:

.. rst-class:: numbered-step

Install dependencies
********************

Next, use a package manager to install required support tools. Python 3
and its package manager, pip, are used extensively by Zephyr for
installing and running scripts used to compile, build, and run Zephyr
applications.

We'll also install Zephyr's multi-purpose west tool.

.. tabs::

   .. group-tab:: Ubuntu

      #. Use the ``apt`` package manager to install these tools:

         .. code-block:: bash

            sudo apt install --no-install-recommends git cmake ninja-build gperf \
              ccache dfu-util device-tree-compiler wget \
              python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
              make gcc gcc-multilib g++-multilib libsdl2-dev

      #. Verify the version of cmake installed on your system using::

            cmake --version

         If it's not version 3.13.1 or higher, follow these steps to
         add the `kitware third-party apt repository <https://apt.kitware.com/>`__
         to get an updated version of cmake.

         a) Add the kitware signing key to apt:

            .. code-block:: bash

               wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -

         b) Add the kitware repo corresponding to the Ubuntu 18.04 LTS release:

            .. code-block:: bash

               sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'

         c) Then install the updated cmake using the usual apt commands:

            .. code-block:: bash

               sudo apt update
               sudo apt install cmake

      #. Install west:

         .. code-block:: bash

            pip3 install --user -U west
            echo 'export PATH=~/.local/bin:"$PATH"' >> ~/.bashrc
            source ~/.bashrc

         The pip3 ``--user`` option puts installed Python packages into your
         ``~/.local/bin folder`` so we'll need to add this to the PATH
         so these packages will be found.  Adding the PATH specification to your
         ``.bashrc`` file ensures this setting is permanent.

   .. group-tab:: macOS

      #. On macOS, install :program:`Homebrew` by following instructions on the `Homebrew
         site`_, and as shown here. Homebrew is a free and open-source package management system that
         simplifies installing software on macOS.  While installing Homebrew,
         you may be prompted to install additional missing dependencies; please follow
         any such instructions as well.

         .. code-block:: bash

            /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

      #. Then, install these host dependencies with the ``brew`` command:

         .. code-block:: bash

            brew install cmake ninja gperf ccache dfu-util qemu dtc python3

      #. Install west:

         .. code-block:: bash

            pip3 install west

      .. _Homebrew site: https://brew.sh/

   .. group-tab:: Windows

      .. note:: Currently, the built-in `Windows Subsystem for Linux (WSL)
         <https://msdn.microsoft.com/en-us/commandline/wsl/install_guide>`__
         doesn't support flashing your application to the board.  As such,
         we don't recommend using WSL yet.

      These instructions assume you are using the Windows ``cmd.exe``
      command prompt. Some of the details, such as setting environment
      variables, may differ if you are using PowerShell.

      An easy way to install native Windows dependencies is to first install
      `Chocolatey`_, a package manager for Windows.  If you prefer to install
      dependencies manually, you can also download the required programs from their
      respective websites and verify they can be found on your PATH.

      |p|

      #. Install :program:`Chocolatey` by following the instructions on the
         `Chocolatey install`_ page.

      #. Open a command prompt (``cmd.exe``) as an **Administrator** (press the
         Windows key, type "cmd.exe" in the prompt, then right-click the result and
         choose "Run as Administrator").

      #. Disable global confirmation to avoid having to confirm
         installation of individual programs:

         .. code-block:: console

            choco feature enable -n allowGlobalConfirmation

      #. Install CMake:

         .. code-block:: console

            choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'

      #. Install the rest of the tools:

         .. code-block:: console

            choco install git python ninja dtc-msys2 gperf

      #. Close the Administrator command prompt window and open a
         regular command prompt window to continue..

      #. Install west:

         .. code-block:: bash

            pip3 install west


.. _Chocolatey: https://chocolatey.org/
.. _Chocolatey install: https://chocolatey.org/install

.. _get_the_code:
.. _clone-zephyr:

.. rst-class:: numbered-step

Get the source code
*******************

Zephyr's multi-purpose west tool simplifies getting the Zephyr
project git repositories and external modules used by Zephyr.
Clone all of Zephyr's git repositories in a new :file:`zephyrproject`
directory using west:

.. tabs::

   .. group-tab:: Ubuntu

      .. code-block:: bash

         cd ~
         west init zephyrproject
         cd zephyrproject
         west update

   .. group-tab:: macOS

      .. code-block:: bash

         cd ~
         west init zephyrproject
         cd zephyrproject
         west update

   .. group-tab:: Windows

      .. code-block:: bat

         cd %HOMEPATH%
         west init zephyrproject
         cd zephyrproject
         west update

.. rst-class:: numbered-step

Export Zephyr CMake package
***************************

Exporting Zephyr as a :ref:`cmake_pkg` makes it possible for CMake to automatically find and load
boilerplate code for building a Zephyr application.

Zephyr CMake package is exported with the following command

.. tabs::

   .. group-tab:: Ubuntu

      .. code-block:: bash

         west zephyr-export

   .. group-tab:: macOS

      .. code-block:: bash

         west zephyr-export

   .. group-tab:: Windows

      .. code-block:: bat

         west zephyr-export

.. _install_py_requirements:

.. rst-class:: numbered-step

Install needed Python packages
******************************

The Zephyr source folders we downloaded contain a ``requirements.txt`` file
that we'll use to install additional Python tools used by the Zephyr
project:

.. tabs::

   .. group-tab:: Ubuntu

      .. code-block:: bash

         pip3 install --user -r ~/zephyrproject/zephyr/scripts/requirements.txt

   .. group-tab:: macOS

      .. code-block:: bash

         pip3 install -r ~/zephyrproject/zephyr/scripts/requirements.txt

   .. group-tab:: Windows

      .. code-block:: bat

         pip3 install -r %HOMEPATH%\zephyrproject\zephyr\scripts\requirements.txt

.. _gs_python_deps:

.. rst-class:: numbered-step

Install Software Development Toolchain
**************************************

A toolchain includes necessary tools used to build Zephyr applications
including: compiler, assembler, linker, and their dependencies.


.. _Zephyr SDK Downloads: https://github.com/zephyrproject-rtos/sdk-ng/releases

.. tabs::

   .. group-tab:: Ubuntu

      Zephyr's Software Development Kit (SDK) contains necessary Linux
      development tools to build Zephyr on all supported architectures.
      Additionally, it includes host tools such as custom QEMU binaries and a
      host compiler.

      |p|

      #. Download the latest SDK as a self-extracting installation binary:

         .. code-block:: bash

            cd ~
            wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.11.2/zephyr-sdk-0.11.2-setup.run

      #. Run the installation binary, installing the SDK in your home
         folder :file:`~/zephyr-sdk-0.11.2`:

         .. code-block:: bash

            chmod +x zephyr-sdk-0.11.2-setup.run
            ./zephyr-sdk-0.11.2-setup.run -- -d ~/zephyr-sdk-0.11.2

      #. Set environment variables to let the build system know where to
         find the toolchain programs:

         .. code-block:: bash

            export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
            export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk-0.11.2

      The SDK contains a udev rules file that provides information
      needed to identify boards and grant hardware access permission to flash
      tools.  Install these udev rules with these commands:

      .. code-block:: bash

         sudo cp ${ZEPHYR_SDK_INSTALL_DIR}/sysroots/x86_64-pokysdk-linux/usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d
         sudo udevadm control --reload

   .. group-tab:: macOS

      #. The Zephyr SDK is not supported on macOS.  See instructions for
         :ref:`installing 3rd-party toolchains<gs_toolchain>`.

      #. Do not forget to set environment variables (ZEPHYR_TOOLCHAIN_VARIANT and toolchain specific ones)
         to let the build system know where to find the toolchain programs.

   .. group-tab:: Windows

      #. The Zephyr SDK is not supported on Windows.  See instructions for
         :ref:`installing 3rd-party toolchains<gs_toolchain>`.

      #. Do not forget to set environment variables (ZEPHYR_TOOLCHAIN_VARIANT and toolchain specific ones)
         to let the build system know where to find the toolchain programs.


.. _getting_started_run_sample:

.. rst-class:: numbered-step

Build the Blinky Application
****************************

The sample :ref:`blinky-sample` blinks an LED on the target board.  By
building and running it, we can verify that the environment and tools
are properly set up for Zephyr development.

   .. note:: This sample is compatible with most boards supported by
      Zephyr, but not all of them. See the :ref:`blinky sample requirements
      <blinky-sample-requirements>` for more information. If this sample is not
      compatible with your board, a good alternative to try is the
      :ref:`Hello World sample <hello_world>`.

#. Build the blinky sample. Specify **your board name**
   (see :ref:`boards`) in the command below:

   .. code-block:: bash

      west build -p auto -b <your-board-name> samples/basic/blinky

   This west command uses the ``-p auto`` parameter to automatically
   clean out any byproducts from a previous build if needed, useful if
   you try building another sample.

.. rst-class:: numbered-step

Flash and Run the Application
*****************************

#. Connect a USB cable between the board and your development computer.
   (Refer to the specific :ref:`boards` documentation if you're not sure
   which connector to use on the board.)
#. If there's a switch, turn the board on.
#. Flash the blinky application you just built using the command:

   .. tabs::

      .. group-tab:: Ubuntu

         .. code-block:: bash

            west flash

         If the flash command fails, and you've checked your
         board is powered on and connected to the right on-board USB connector,
         verify you've granted needed access permission by
         :ref:`setting-udev-rules`.

      .. group-tab:: macOS

         .. code-block:: bash

            west flash

      .. group-tab:: Windows

         .. code-block:: bat

            west flash


The application will start running and you'll see blinky in action. The
actual blinking LED location is board specific.

.. figure:: img/ReelBoard-Blinky.gif
   :width: 400px
   :name: reelboard-blinky

   Phytec reel_board running blinky


Next Steps
**********

Now that you've got the blinky sample running, here are some next steps
for exploring Zephyr:

* Try some other :ref:`samples-and-demos` that demonstrate Zephyr
  capabilities.
* Learn about :ref:`application` and more details about :ref:`west`.
* Check out :ref:`beyond-GSG` for information about advanced setup
  alternatives and issues.
* Discover :ref:`project-resources` for getting help from the Zephyr
  community.
