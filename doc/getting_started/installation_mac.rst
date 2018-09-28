.. _installing_zephyr_mac:

Development Environment Setup on macOS
######################################

This section describes how to set up a macOS development system.

After completing these steps, you will be able to compile and run your Zephyr
applications on the following macOS version:

* Mac OS X 10.11 (El Capitan)
* macOS Sierra 10.12

Developing for Zephyr on macOS generally requires you to build the
toolchain yourself. However, if there is already an macOS toolchain for your
target architecture you can use it directly.

Using a 3rd Party toolchain
***************************

If a toolchain is available for the architecture you plan to build for, then
you can use it as explained in: :ref:`third_party_x_compilers`.

An example of an available 3rd party toolchain is GCC ARM Embedded for the
Cortex-M family of cores.

.. _mac_requirements:

Installing Requirements and Dependencies
****************************************

To install the software components required to build the Zephyr kernel on a
Mac, you will need to build a cross compiler for the target devices you wish to
build for and install tools that the build system requires.

.. note::
   Minor version updates of the listed required packages might also
   work.

Before proceeding with the build, ensure your OS is up to date.

First, install the :program:`Homebrew` (The missing package manager for
macOS). Homebrew is a free and open-source software package management system
that simplifies the installation of software on Apple's macOS operating
system.

To install :program:`Homebrew`, visit the `Homebrew site`_ and follow the
installation instructions on the site.

To complete the Homebrew installation, you might be prompted to install some
missing dependency. If so, follow please follow the instructions provided.

After Homebrew is successfully installed, install the following tools using
the brew command line.

.. note::
   Zephyr requires Python 3 in order to be built. Since macOS comes bundled
   only with Python 2, we will need to install Python 3 with Homebrew. After
   installing it you should have the macOS-bundled Python 2 in ``/usr/bin/``
   and the Homebrew-provided Python 3 in ``/usr/local/bin``.

Install tools to build Zephyr binaries:

.. code-block:: console

   brew install cmake ninja dfu-util doxygen qemu dtc python3 gperf
   cd ~/zephyr   # or to the folder where you cloned the zephyr repo
   pip3 install --user -r scripts/requirements.txt

.. note::
   If ``pip3`` does not seem to have been installed correctly use
   ``brew reinstall python3`` in order to reinstall it.

Source :file:`zephyr-env.sh` wherever you have cloned the Zephyr Git repository:

.. code-block:: console

   unset ZEPHYR_SDK_INSTALL_DIR
   cd <zephyr git clone location>
   source zephyr-env.sh

Finally, assuming you are using a 3rd-party toolchain you can try building the :ref:`hello_world` sample to check things out.

To build for the ARM-based Nordic nRF52 Development Kit:

.. zephyr-app-commands::
  :zephyr-app: samples/hello_world
  :board: nrf52_pca10040
  :host-os: unix
  :goals: build

Install tools to build Zephyr documentation:

.. code-block:: console

   brew install mactex librsvg
   tlmgr install latexmk
   tlmgr install collection-fontsrecommended

.. _setting_up_mac_toolchain:

Setting Up the Toolchain
************************

In case a toolchain is not available for the board you are using, you can build
a toolchain from scratch using crosstool-NG. Follow the steps on the
crosstool-NG website to `prepare your host
<http://crosstool-ng.github.io/docs/os-setup/>`_

Follow the `Zephyr SDK with Crosstool NG instructions <https://github.com/zephyrproject-rtos/sdk-ng/blob/master/README.md>`_ to build
the toolchain for various architectures. You will need to clone the ``sdk-ng``
repo and run the following command::

   ./go.sh <arch>

.. note::
   Currently only i586 and arm builds are verified.


Repeat the step for all architectures you want to support in your environment.

To use the toolchain with Zephyr, export the following environment variables
and use the target location where the toolchain was installed, type:

.. code-block:: console

   export ZEPHYR_TOOLCHAIN_VARIANT=xtools
   export XTOOLS_TOOLCHAIN_PATH=/Volumes/CrossToolNGNew/build/output/


To use the same toolchain in new sessions in the future you can set the
variables in the file :file:`${HOME}/.zephyrrc`, for example:

.. code-block:: console

   cat <<EOF > ~/.zephyrrc
   export XTOOLS_TOOLCHAIN_PATH=/Volumes/CrossToolNGNew/build/output/
   export ZEPHYR_TOOLCHAIN_VARIANT=xtools
   EOF

.. note:: In previous releases of Zephyr, the ``ZEPHYR_TOOLCHAIN_VARIANT``
          variable was called ``ZEPHYR_GCC_VARIANT``.

.. _Homebrew site: http://brew.sh/

.. _crosstool-ng site: http://crosstool-ng.org

