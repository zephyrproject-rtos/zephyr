.. _toolchain_armclang:

Arm Compiler 6
##############

#. Download and install a development suite containing the `Arm Compiler 6`_
   for your operating system.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``armclang``.
   - Set :envvar:`ARMCLANG_TOOLCHAIN_PATH` to the toolchain installation
     directory.

#. For a FlexNet license, set the :envvar:`ARMLMD_LICENSE_FILE` environment
   variable to your license file or server. Arm also documents a
   `Community license for open-source CI`_ using its ``armlm`` activation tool.

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

Local ISR table generation
**************************

The Cortex-M integration enables :kconfig:option:`CONFIG_ISR_TABLES_LOCAL_DECLARATION`
by default on supported configurations. Set it to ``n`` to select the legacy generator.

With local declarations, armlink retains executable relocations and places registration
metadata in a dedicated overlay. Before the ELF is consumed by the next build step,
``isr_metadata_finalize.py`` makes that metadata nonallocated and nonloadable. The script
rejects overlays containing runtime sections or references to metadata from runtime code.
All other loaded bytes, addresses, and permissions must remain unchanged.

``armlink_symbol_export.py`` restores public names removed by armlink's ``RESOLVE``
steering commands. Runtime references are resolved by armlink; the script exports values
from that same completed link into the nonallocated ELF symbol table. It preserves
absolute symbols as absolute symbols and verifies that the load image is unchanged.

The native Arm CI job uses a fixed Compiler 6.24 release and the official Community
activation action. It runs the default configuration, array and switch tables, an
LTO-enabled kernel, dynamic and shared registration, and NVIC-triggered ordinary and
direct interrupts on QEMU.
The same test runner is :file:`scripts/ci/run_isr_table_armclang.py`.

Runtime references are checked through retained relocations. ARM branch veneers and
branch addends that hide the original target are rejected. TARGET1 entries are checked
as both ABS32 and REL32; the build fails if either interpretation reaches metadata.

.. _Arm Compiler 6: https://developer.arm.com/tools-and-software/embedded/arm-compiler/downloads/version-6

.. _Community license for open-source CI: https://arm-software.github.io/AVH/main/infrastructure/html/avh_gh_actions.html
