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

         If your host architecture is 64-bit ARM (Apple Silicon, also known as M1), replace
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

.. _Zephyr SDK Releases: https://github.com/zephyrproject-rtos/sdk-ng/tags
.. _Zephyr SDK Version Compatibility Matrix: https://github.com/zephyrproject-rtos/sdk-ng/wiki/Zephyr-SDK-Version-Compatibility-Matrix

.. toolchain_zephyr_sdk_install_end
