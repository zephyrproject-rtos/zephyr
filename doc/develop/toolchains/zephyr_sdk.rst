.. _toolchain_zephyr_sdk:

Zephyr SDK
##########

The Zephyr Software Development Kit (SDK) contains toolchains for each of
Zephyr's supported architectures. It also includes additional host tools, such
as custom QEMU and OpenOCD.

Use of the Zephyr SDK is highly recommended and may even be required under
certain conditions (for example, running tests in QEMU for some architectures).

Supported architectures
***********************

The Zephyr SDK supports the following target architectures:

* ARC (32-bit and 64-bit; ARCv1, ARCv2, ARCv3)
* ARM (32-bit and 64-bit; ARMv6, ARMv7, ARMv8; A/R/M Profiles)
* MIPS (32-bit and 64-bit)
* Nios II
* RISC-V (32-bit and 64-bit; RV32I, RV32E, RV64I)
* x86 (32-bit and 64-bit)
* Xtensa

.. _toolchain_zephyr_sdk_bundle_variables:

Installation bundle and variables
*********************************

The Zephyr SDK bundle supports all major operating systems (Linux, macOS and
Windows) and is delivered as a compressed file.
The installation consists of extracting the file and running the included setup
script. Additional OS-specific instructions are described in the sections below.

If no toolchain is selected, the build system looks for Zephyr SDK and uses the toolchain
from there. You can enforce this by setting the environment variable
:envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``zephyr``.

If you install the Zephyr SDK outside any of the default locations (listed in
the operating system specific instructions below) and you want automatic discovery
of the Zephyr SDK, then you must register the Zephyr SDK in the CMake package registry
by running the setup script. If you decide not to register the Zephyr SDK in the CMake registry,
then the :envvar:`ZEPHYR_SDK_INSTALL_DIR` can be used to point to the Zephyr SDK installation
directory.

You can also set :envvar:`ZEPHYR_SDK_INSTALL_DIR` to point to a directory
containing multiple Zephyr SDKs, allowing for automatic toolchain selection. For
example, you can set ``ZEPHYR_SDK_INSTALL_DIR`` to ``/company/tools``, where the
``company/tools`` folder contains the following subfolders:

* ``/company/tools/zephyr-sdk-0.13.2``
* ``/company/tools/zephyr-sdk-a.b.c``
* ``/company/tools/zephyr-sdk-x.y.z``

This allows the Zephyr build system to choose the correct version of the SDK,
while allowing multiple Zephyr SDKs to be grouped together at a specific path.

.. _toolchain_zephyr_sdk_compatibility:

Zephyr SDK version compatibility
********************************

In general, the Zephyr SDK version referenced in this page should be considered
the recommended version for the corresponding Zephyr version.

For the full list of compatible Zephyr and Zephyr SDK versions, refer to the
`Zephyr SDK Version Compatibility Matrix`_.

.. _toolchain_zephyr_sdk_install:

Zephyr SDK installation
***********************

.. toolchain_zephyr_sdk_install_start

.. note:: You can change |sdk-version-literal| to another version in the instructions below
          if needed; the `Zephyr SDK Releases`_ page contains all available
          SDK releases.

.. note:: If you want to uninstall the SDK, you may simply remove the directory
          where you installed it.

.. tabs::

   .. group-tab:: Ubuntu

      .. _ubuntu_zephyr_sdk:

      #. Download and verify the `Zephyr SDK bundle`_:

         .. parsed-literal::

            cd ~
            wget |sdk-url-linux|
            wget -O - |sdk-url-linux-sha| | shasum --check --ignore-missing

         If your host architecture is 64-bit ARM (for example, Raspberry Pi), replace ``x86_64``
         with ``aarch64`` in order to download the 64-bit ARM Linux SDK.

      #. Extract the Zephyr SDK bundle archive:

         .. parsed-literal::

            tar xvf zephyr-sdk- |sdk-version-trim| _linux-x86_64.tar.xz

         .. note::
            It is recommended to extract the Zephyr SDK bundle at one of the following locations:

            * ``$HOME``
            * ``$HOME/.local``
            * ``$HOME/.local/opt``
            * ``$HOME/bin``
            * ``/opt``
            * ``/usr/local``

            The Zephyr SDK bundle archive contains the ``zephyr-sdk-<version>``
            directory and, when extracted under ``$HOME``, the resulting
            installation path will be ``$HOME/zephyr-sdk-<version>``.

      #. Run the Zephyr SDK bundle setup script:

         .. parsed-literal::

            cd zephyr-sdk- |sdk-version-ltrim|
            ./setup.sh

         .. note::
            You only need to run the setup script once after extracting the Zephyr SDK bundle.

            You must rerun the setup script if you relocate the Zephyr SDK bundle directory after
            the initial setup.

      #. Install `udev <https://en.wikipedia.org/wiki/Udev>`_ rules, which
         allow you to flash most Zephyr boards as a regular user:

         .. parsed-literal::

            sudo cp ~/zephyr-sdk- |sdk-version-trim| /sysroots/x86_64-pokysdk-linux/usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d
            sudo udevadm control --reload

   .. group-tab:: macOS

      .. _macos_zephyr_sdk:

      #. Download and verify the `Zephyr SDK bundle`_:

         .. parsed-literal::

            cd ~
            curl -L -O |sdk-url-macos|
            curl -L |sdk-url-macos-sha| | shasum --check --ignore-missing

         If your host architecture is 64-bit ARM (Apple Silicon), replace
         ``x86_64`` with ``aarch64`` in order to download the 64-bit ARM macOS SDK.

      #. Extract the Zephyr SDK bundle archive:

         .. parsed-literal::

            tar xvf zephyr-sdk- |sdk-version-trim| _macos-x86_64.tar.xz

         .. note::
            It is recommended to extract the Zephyr SDK bundle at one of the following locations:

            * ``$HOME``
            * ``$HOME/.local``
            * ``$HOME/.local/opt``
            * ``$HOME/bin``
            * ``/opt``
            * ``/usr/local``

            The Zephyr SDK bundle archive contains the ``zephyr-sdk-<version>``
            directory and, when extracted under ``$HOME``, the resulting
            installation path will be ``$HOME/zephyr-sdk-<version>``.

      #. Run the Zephyr SDK bundle setup script:

         .. parsed-literal::

            cd zephyr-sdk- |sdk-version-ltrim|
            ./setup.sh

         .. note::
            You only need to run the setup script once after extracting the Zephyr SDK bundle.

            You must rerun the setup script if you relocate the Zephyr SDK bundle directory after
            the initial setup.

   .. group-tab:: Windows

      .. _windows_zephyr_sdk:

      #. Open a ``cmd.exe`` terminal window **as a regular user**

      #. Download the `Zephyr SDK bundle`_:

         .. parsed-literal::

            cd %HOMEPATH%
            wget |sdk-url-windows|

      #. Extract the Zephyr SDK bundle archive:

         .. parsed-literal::

            7z x zephyr-sdk- |sdk-version-trim| _windows-x86_64.7z

         .. note::
            It is recommended to extract the Zephyr SDK bundle at one of the following locations:

            * ``%HOMEPATH%``
            * ``%PROGRAMFILES%``

            The Zephyr SDK bundle archive contains the ``zephyr-sdk-<version>``
            directory and, when extracted under ``%HOMEPATH%``, the resulting
            installation path will be ``%HOMEPATH%\zephyr-sdk-<version>``.

      #. Run the Zephyr SDK bundle setup script:

         .. parsed-literal::

            cd zephyr-sdk- |sdk-version-ltrim|
            setup.cmd

         .. note::
            You only need to run the setup script once after extracting the Zephyr SDK bundle.

            You must rerun the setup script if you relocate the Zephyr SDK bundle directory after
            the initial setup.

.. toolchain_zephyr_sdk_install_end

.. _toolchain_zephyr_sdk_shared_env:

Zephyr SDK Installation in Shared Environments
**********************************************

In shared environments, such as common and distributed build infrastructure, it can be
advantageous to install the Zephyr SDK and Python dependencies in a shared location.

This has several benefits:

* Reduced disk space used compared to multiple Zephyr SDKs and Python virtual environments
* Reduced on-boarding time for new Zephyr developers
* Simplified installation onto neighboring build machines (local and remote)
* Supports multiple simultaneous installations of different SDK versions side-by-side

Assumptions:

* A UNIX-like operating system (e.g. Linux, macOS)
* The installer has super-user privileges (e.g. with ``sudo``)
* The version of the Zephyr SDK installed is represented by ``$SDK_VERSION`` (e.g. 0.17.0)
* The SDK is installed to ``/opt/zephyr/sdk/$SDK_VERSION``
* The version of the Python virtual environment is represented by ``$VENV_VERSION`` (e.g. 20250101)
* The virtual environment is installed to ``/opt/zephyr/venv/$VENV_VERSION``

The same steps in :ref:`Getting Started Guide <getting_started>` are followed
with minor variations.

1. Create the shared directory and set the owner to the current user

   .. code-block:: console

      sudo mkdir -p /opt/zephyr/sdk/$SDK_VERSION
      sudo chown $UID:$UID /opt/zephyr/sdk/$SDK_VERSION

2. Create a shared Python virtual environment (in :ref:`this step <gs_python_deps>`)

   .. code-block:: console

      python3 -m venv /opt/zephyr/venv/$VENV_VERSION
      source /opt/zephyr/venv/$VENV_VERSION/bin/activate

3. Install the Zephyr SDK (in :ref:`this step <gs_install_zephyr_sdk>`)

   .. code-block:: console

      west sdk install -d /opt/zephyr/sdk/$SDK_VERSION

4. Change the owner to root

   .. code-block:: console

      sudo chown -R 0:0 /opt/zephyr/sdk/$SDK_VERSION
      sudo chown -R 0:0 /opt/zephyr/venv/$VENV_VERSION

When opening as a new or existing user, export the necessary environment
variables before building Zephyr as usual.

   .. code-block:: console

      cd ~/zephyrproject/zephyr
      source /opt/zephyr/venv/$VENV_VERSION/bin/activate
      export ZEPHYR_BASE=$PWD
      export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
      export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr/sdk/$SDK_VERSION

Depending on the topology of build infrastructure, it may be preferable to simply ``rsync``
the ``/opt/zephyr/sdk/$SDK_VERSION`` directory to other build machines. Alternatively, use
``/opt/zephyr/sdk/$SDK_VERSION`` to create packages using the package manager of choice.

.. _gs_package_managers:

Guidelines for Package Managers
*******************************

For those who wish to create redistributable packages (with e.g.
`APT <https://en.wikipedia.org/wiki/APT_(software)>`_ or
`RPM <https://en.wikipedia.org/wiki/RPM_Package_Manager>`_) from the Zephyr SDK and Python
dependencies, please follow the general guidelines below. These guidelines support multiple
simultaneous installations of different SDK versions side-by-side, which can be helpful when
building for different Zephyr releases or when evaluating new Zephyr SDK releases.

Assumptions:

* A UNIX-like operating system (e.g. Linux, macOS)
* The shared SDK is installed in ``/opt/zephyr/sdk/$SDK_VERSION``
* The version of the Zephyr SDK installed is represented by ``$SDK_VERSION`` (e.g. 0.17.0)
* The Python virtual environment is installed to ``/opt/zephyr/venv/$VENV_VERSION``
* A Zephyr toolchain component by target architecture is represented by ``$TARGET`` (e.g. ``aarch64``)

Suggested packages:

* ``zephyr-pyvenv-$VENV_VERSION``:

  * the tree structure under ``/opt/zephyr/venv/$VENV_VERSION``
  * a time-based snapshot of all required python packages for building Zephyr at a specific time.
  * may apply to multiple Zephyr and Zephyr SDK releases.

* ``zephyr-sdk-$SDK_VERSION``:

  * a top-level package that pulls in other all other packages for a given Zephyr SDK release
  * does not install any files directly
  * optional, but recommended for ease of use

* ``zephyr-sdk-$SDK_VERSION-base``:

  * the base layout for the installed Zephyr SDK version
  * includes files under ``/opt/zephyr/sdk/$SDK_VERSION``
  * limited to cmake rules, scripts, version files, etc
  * does not include toolchain components
  * does not include host tools
  * does not include Python virtual environment

* ``zephyr-sdk-$SDK_VERSION-hosttools``:

  * the host tools for the installed Zephyr SDK version (if applicable)
  * includes files under ``/opt/zephyr/sdk/$SDK_VERSION/sysroots``
  * for hosts without complete host tools support, this package may be empty

* ``zephyr-sdk-$SDK_VERSION-toolchain-$TARGET``:

  * the ``$TARGET``-specific toolchain component of Zephyr SDK ``$SDK_VERSION``
  * includes files under e.g. ``/opt/zephyr/sdk/$SDK_VERSION/$TARGET-zephyr-elf``

.. _Zephyr SDK Releases: https://github.com/zephyrproject-rtos/sdk-ng/tags
.. _Zephyr SDK Version Compatibility Matrix: https://github.com/zephyrproject-rtos/sdk-ng/wiki/Zephyr-SDK-Version-Compatibility-Matrix
