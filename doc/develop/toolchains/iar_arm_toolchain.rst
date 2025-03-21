.. _toolchain_iar_arm:

IAR Arm Toolchain
#################

#. Download and install a release of  `IAR Arm Toolchain`_ (EWARM/CXARM) on your host.

.. note::
   As of now, a Development version of the IAR build tools for Arm is required to work with Zephyr.
   It is distributed to selected partners and customers for evaluation. If you are interested in being
   part of this program, please send a request to the IAR FAE team at fae.emea@iar.com.

#. Make sure you have :ref:`Zephyr SDK <toolchain_zephyr_sdk>` installed on your host.

.. note::
   A Zephyr SDK is used as a source of tools like device tree compiler (DTC), QEMU, etc… Even though
   IAR Arm toolchain is used for Zephyr RTOS build, still the GNU preprocessor & GNU objcopy might
   be used for some steps like device tree preprocessing and .bin file generation.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``iar``.
   - Set :envvar:`IAR_TOOLCHAIN_PATH` to the toolchain installation directory.

#. The IAR Toolchain needs the :envvar:`IAR_LMS_BEARER_TOKEN` environment
   variable to be set to a valid ``license bearer token``.

For example:

   .. code-block:: bash

      # Linux (default installation path):
      export IAR_TOOLCHAIN_PATH=/opt/iarsystems/bxarm/arm
      export ZEPHYR_TOOLCHAIN_VARIANT=iar
      export IAR_LMS_BEARER_TOKEN="<BEARER-TOKEN>"

   .. code-block:: batch

      # Windows:
      set IAR_TOOLCHAIN_PATH=c:\<path>\arm
      set ZEPHYR_TOOLCHAIN_VARIANT=iar
      set IAR_LMS_BEARER_TOKEN="<BEARER-TOKEN>"

.. note::

   The IAR Toolchain uses ``ilink`` for linking. This is incompatible with Zephyr’s
   linker script template, which works with GNU ld. Zephyr’s IAR Arm Toolchain depends on
   Zephyr’s CMake linker script generator, which supports generating icf-files.
   Basic icf-file support is in place, but there are still areas which are not fully
   supported by the CMake linker script generator.

.. note::

   The IAR Toolchain uses the GNU Assembler which is distributed with the Zephyr SDK
   for ``.S-files``.

.. note::

   Some Zephyr subsystems or modules may also contain C or assembly code that relies
   on GNU intrinsics and have not yet been updated to work fully with ``iar``.

.. _IAR Arm Toolchain: https://www.iar.com/products/architectures/arm/
