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

.. _toolchain_zephyr_sdk_install_linux:

Install Zephyr SDK on Linux
***************************

#. Download and verify the `latest Zephyr SDK bundle`_:

   .. code-block:: bash

      wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.0/zephyr-sdk-0.16.0_linux-x86_64.tar.xz
      wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.0/sha256.sum | shasum --check --ignore-missing

   You can change ``0.16.0`` to another version if needed; the `Zephyr SDK
   Releases`_ page contains all available SDK releases.

   If your host architecture is 64-bit ARM (for example, Raspberry Pi), replace
   ``x86_64`` with ``aarch64`` in order to download the 64-bit ARM Linux SDK.

#. Extract the Zephyr SDK bundle archive:

   .. code-block:: bash

      cd <sdk download directory>
      tar xvf zephyr-sdk-0.16.0_linux-x86_64.tar.xz

#. Run the Zephyr SDK bundle setup script:

   .. code-block:: bash

      cd zephyr-sdk-0.16.0
      ./setup.sh

   If this fails, make sure Zephyr's dependencies were installed as described
   in :ref:`Install Requirements and Dependencies <linux_requirements>`.

If you want to uninstall the SDK, remove the directory where you installed it.
If you relocate the SDK directory, you need to re-run the setup script.

.. note::
   It is recommended to extract the Zephyr SDK bundle at one of the following
   default locations:

   * ``$HOME``
   * ``$HOME/.local``
   * ``$HOME/.local/opt``
   * ``$HOME/bin``
   * ``/opt``
   * ``/usr/local``

   The Zephyr SDK bundle archive contains the ``zephyr-sdk-0.16.0`` directory and, when
   extracted under ``$HOME``, the resulting installation path will be
   ``$HOME/zephyr-sdk-0.16.0``.

.. _toolchain_zephyr_sdk_install_macos:

Install Zephyr SDK on macOS
***************************

#. Download and verify the `latest Zephyr SDK bundle`_:

   .. code-block:: bash

      cd ~
      wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.0/zephyr-sdk-0.16.0_macos-x86_64.tar.xz
      wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.0/sha256.sum | shasum --check --ignore-missing

   If your host architecture is 64-bit ARM (Apple Silicon, also known as M1), replace
   ``x86_64`` with ``aarch64`` in order to download the 64-bit ARM macOS SDK.

#. Extract the Zephyr SDK bundle archive:

   .. code-block:: bash

      tar xvf zephyr-sdk-0.16.0_macos-x86_64.tar.xz

   .. note::
      It is recommended to extract the Zephyr SDK bundle at one of the following
      default locations:

      * ``$HOME``
      * ``$HOME/.local``
      * ``$HOME/.local/opt``
      * ``$HOME/bin``
      * ``/opt``
      * ``/usr/local``

      The Zephyr SDK bundle archive contains the ``zephyr-sdk-0.16.0`` directory and, when
      extracted under ``$HOME``, the resulting installation path will be
      ``$HOME/zephyr-sdk-0.16.0``.

#. Run the Zephyr SDK bundle setup script:

   .. code-block:: bash

      cd zephyr-sdk-0.16.0
      ./setup.sh

   .. note::
      You only need to run the setup script once after extracting the Zephyr SDK bundle.

      You must rerun the setup script if you relocate the Zephyr SDK bundle directory after
      the initial setup.

.. _toolchain_zephyr_sdk_install_windows:

Install Zephyr SDK on Windows
*****************************

#. Open a ``cmd.exe`` window by pressing the Windows key typing "cmd.exe".

#. Download the `latest Zephyr SDK bundle`_:

   .. code-block:: console

      cd %HOMEPATH%
      wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.0/zephyr-sdk-0.16.0_windows-x86_64.7z

#. Extract the Zephyr SDK bundle archive:

   .. code-block:: console

      7z x zephyr-sdk-0.16.0_windows-x86_64.7z

   .. note::
      It is recommended to extract the Zephyr SDK bundle at one of the following
      default locations:

      * ``%HOMEPATH%``
      * ``%PROGRAMFILES%``

      The Zephyr SDK bundle archive contains the ``zephyr-sdk-0.16.0`` directory and, when
      extracted under ``%HOMEPATH%``, the resulting installation path will be
      ``%HOMEPATH%\zephyr-sdk-0.16.0``.

#. Run the Zephyr SDK bundle setup script:

   .. code-block:: console

      cd zephyr-sdk-0.16.0
      setup.cmd

   .. note::
      You only need to run the setup script once after extracting the Zephyr SDK bundle.

      You must rerun the setup script if you relocate the Zephyr SDK bundle directory after
      the initial setup.

.. _latest Zephyr SDK bundle: https://github.com/zephyrproject-rtos/sdk-ng/releases
.. _Zephyr SDK Releases: https://github.com/zephyrproject-rtos/sdk-ng/releases
