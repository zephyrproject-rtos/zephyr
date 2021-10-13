.. _third_party_x_compilers:

3rd Party Toolchains
####################

A "3rd party toolchain" is an officially supported toolchain provided by an
external organization. Several of these are available.

.. _toolchain_gnuarmemb:

GNU Arm Embedded
****************

   .. warning::

      Do not install the toolchain into a path with spaces.

#. Download and install a `GNU Arm Embedded`_ build for your operating system
   and extract it on your file system.

   .. note::

      On Windows, we'll assume you install into the directory
      :file:`C:\\gnu_arm_embedded`.

   .. warning::

      On macOS Catalina or later you might need to :ref:`change a security
      policy <mac-gatekeeper>` for the toolchain to be able to run from the
      terminal.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``gnuarmemb``.
   - Set :envvar:`GNUARMEMB_TOOLCHAIN_PATH` to the toolchain installation
     directory.

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`GNUARMEMB_TOOLCHAIN_PATH` values may be different on your system):

   .. code-block:: console

      # Linux, macOS:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      gnuarmemb
      $ echo $GNUARMEMB_TOOLCHAIN_PATH
      /home/you/Downloads/gnu_arm_embedded

      # Windows:
      > echo %ZEPHYR_TOOLCHAIN_VARIANT%
      gnuarmemb
      > echo %GNUARMEMB_TOOLCHAIN_PATH%
      C:\gnu_arm_embedded

   .. warning::

      On macOS, if you are having trouble with the suggested procedure, there is an unofficial package on brew that might help you.
      Run ``brew install gcc-arm-embedded`` and configure the variables

      - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``gnuarmemb``.
      - Set :envvar:`GNUARMEMB_TOOLCHAIN_PATH` to the brew installation directory (something like ``/usr/local``)

.. _toolchain_armclang:

Arm Compiler 6
**************

#. Download and install a development suite containing the `Arm Compiler 6`_
   for your operating system.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``armclang``.
   - Set :envvar:`ARMCLANG_TOOLCHAIN_PATH` to the toolchain installation
     directory.

#. The Arm Compiler 6 needs the :envvar:`ARMLMD_LICENSE_FILE` environment
   variable to point to your license file or server.

For example:

   .. code-block:: console

      # Linux, macOS, license file:
      export ARMLMD_LICENSE_FILE=/<path>/license_armds.dat
      # Linux, macOS, license server:
      export ARMLMD_LICENSE_FILE=8224@myserver

      # Windows, license file:
      > set ARMLMD_LICENSE_FILE=c:\<path>\license_armds.dat
      # Windows, license server:
      > set ARMLMD_LICENSE_FILE=8224@myserver

#. If the Arm Compiler 6 was installed as part of an Arm Development Studio, then
   you must set the :envvar:`ARM_PRODUCT_DEF` to point to the product definition file:
   See also: `Product and toolkit configuration <https://developer.arm.com/tools-and-software/software-development-tools/license-management/resources/product-and-toolkit-configuration>`_.
   For example if the Arm Development Studio is installed in:
   ``/opt/armds-2020-1`` with a Gold license, then set :envvar:`ARM_PRODUCT_DEF`
   to point to ``/opt/armds-2020-1/gold.elmap``.

   .. note::

      The Arm Compiler 6 uses ``armlink`` for linking. This is incompatible
      with Zephyr's linker script template, which works with GNU ld. Zephyr's
      Arm Compiler 6 support Zephyr's CMake linker script generator, which
      supports generating scatter files. Basic scatter file support is in
      place, but there are still areas covered in ld templates which are not
      fully supported by the CMake linker script generator.

      Some Zephyr subsystems or modules may also contain C or assembly code
      that relies on GNU intrinsics and have not yet been updated to work fully
      with ``armclang``.

Intel oneAPI Toolkit
*********************

#. Download `Intel oneAPI Base Toolkit
   <https://software.intel.com/content/www/us/en/develop/tools/oneapi/all-toolkits.html>`_

#. Assuming the toolkit is installed in ``/opt/intel/oneApi``, set environment
   using::

        # Linux, macOS:
        export ONEAPI_TOOLCHAIN_PATH=/opt/intel/oneapi
        source $ONEAPI_TOOLCHAIN_PATH/compiler/latest/env/vars.sh

        # Windows:
        > set ONEAPI_TOOLCHAIN_PATH=C:\Users\Intel\oneapi

   To setup the complete oneApi environment, use::

        source  /opt/intel/oneapi/setvars.sh

   The above will also change the python environment to the one used by the
   toolchain and might conflict with what Zephyr uses.

#. Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``oneApi``.

DesignWare ARC MetaWare Development Toolkit (MWDT)
**************************************************

#. You need to have `ARC MWDT
   <https://www.synopsys.com/dw/ipdir.php?ds=sw_metaware>`_ installed on your
   host.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``arcmwdt``.
   - Set :envvar:`ARCMWDT_TOOLCHAIN_PATH` to the toolchain installation
     directory. MWDT installation provides :envvar:`METAWARE_ROOT` so simply set
     :envvar:`ARCMWDT_TOOLCHAIN_PATH` to ``$METAWARE_ROOT/../`` (Linux)
     or ``%METAWARE_ROOT%\..\`` (Windows)

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`ARCMWDT_TOOLCHAIN_PATH` values may be different on your system):

   .. code-block:: console

      # Linux:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      arcmwdt
      $ echo $ARCMWDT_TOOLCHAIN_PATH
      /home/you/ARC/MWDT_2019.12/

      # Windows:
      > echo %ZEPHYR_TOOLCHAIN_VARIANT%
      arcmwdt
      > echo %ARCMWDT_TOOLCHAIN_PATH%
      C:\ARC\MWDT_2019.12\

Crosstool-NG
************

You can build toolchains from source code using crosstool-NG.

#. Follow the steps on the crosstool-NG website to `prepare your host
   <http://crosstool-ng.github.io/docs/os-setup/>`_.

#. Follow the `Zephyr SDK with Crosstool NG instructions
   <https://github.com/zephyrproject-rtos/sdk-ng/blob/master/README.md>`_ to
   build your toolchain. Repeat as necessary to build toolchains for multiple
   target architectures.

   You will need to clone the ``sdk-ng`` repo and run the following command:

   .. code-block:: console

      ./go.sh <arch>

   .. note::

      Currently, only i586 and Arm toolchain builds are verified.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``xtools``.
   - Set :envvar:`XTOOLS_TOOLCHAIN_PATH` to the toolchain build directory.

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`XTOOLS_TOOLCHAIN_PATH` values may be different on your system):

   .. code-block:: console

      # Linux, macOS:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      xtools
      $ echo $XTOOLS_TOOLCHAIN_PATH
      /Volumes/CrossToolNGNew/build/output/

.. _GNU Arm Embedded: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm
.. _crosstool-ng site: http://crosstool-ng.org
.. _Arm Compiler 6: https://developer.arm.com/tools-and-software/embedded/arm-compiler/downloads/version-6
