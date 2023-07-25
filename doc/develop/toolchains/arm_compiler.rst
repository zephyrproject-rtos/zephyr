.. _toolchain_armclang:

Arm Compiler 6
##############

#. Download and install a development suite containing the `Arm Compiler 6`_
   for your operating system.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``armclang``.
   - Set :envvar:`ARMCLANG_TOOLCHAIN_PATH` to the toolchain installation
     directory.

#. The Arm Compiler 6 needs the :envvar:`ARMLMD_LICENSE_FILE` environment
   variable to point to your license file or server.

For example:

   .. code-block:: bash

      # Linux, macOS, license file:
      export ARMLMD_LICENSE_FILE=/<path>/license_armds.dat
      # Linux, macOS, license server:
      export ARMLMD_LICENSE_FILE=8224@myserver

   .. code-block:: batch

      # Windows, license file:
      set ARMLMD_LICENSE_FILE=c:\<path>\license_armds.dat
      # Windows, license server:
      set ARMLMD_LICENSE_FILE=8224@myserver

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

.. _Arm Compiler 6: https://developer.arm.com/tools-and-software/embedded/arm-compiler/downloads/version-6
