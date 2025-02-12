.. _code_data_relocation:

Code And Data Relocation
########################

Overview
********
This feature will allow relocating .text, .rodata, .data, and .bss sections from
required files and place them in the required memory region. The memory region
and file are given to the :ref:`gen_relocate_app.py` script in the form
of a string. This script is always invoked from inside cmake.

This script provides a robust way to re-order the memory contents without
actually having to modify the code.  In simple terms this script will do the job
of ``__attribute__((section("name")))`` for a bunch of files together.

Details
*******
The memory region and file are given to the :ref:`gen_relocate_app.py` script in the form of a string.

An example of such a string is:
``SRAM2:/home/xyz/zephyr/samples/hello_world/src/main.c,SRAM1:/home/xyz/zephyr/samples/hello_world/src/main2.c``

This script is invoked with the following parameters:
``python3 gen_relocate_app.py -i input_string -o generated_linker -c generated_code``

Kconfig :kconfig:option:`CONFIG_CODE_DATA_RELOCATION` option,  when enabled in
``prj.conf``, will invoke the script and do the required relocation.

This script also trigger the generation of ``linker_relocate.ld`` and
``code_relocation.c`` files.  The ``linker_relocate.ld`` file creates
appropriate sections and links the required functions or variables from all the
selected files.

.. note::

   The text section is split into 2 parts in the main linker script. The first
   section will have some info regarding vector tables and other debug related
   info.  The second section will have the complete text section.  This is
   needed to force the required functions and data variables to the correct
   locations.  This is due to the behavior of the linker. The linker will only
   link once and hence this text section had to be split to make room for the
   generated linker script.

The ``code_relocation.c`` file has code that is needed for
initializing data sections, and a copy of the text sections (if XIP).
Also this contains code needed for bss zeroing and
for  data copy operations from ROM to required memory type.

**The procedure to invoke this feature is:**

* Enable :kconfig:option:`CONFIG_CODE_DATA_RELOCATION` in the ``prj.conf`` file

* Inside the ``CMakeLists.txt`` file in the project, mention
  all the files that need relocation.

  ``zephyr_code_relocate(FILES src/*.c LOCATION SRAM2)``

  Where the first argument is the file/files and the second
  argument is the memory where it must be placed.

  .. note::

     function zephyr_code_relocate() can be called  as many times as required.

Additional Configurations
=========================
This section shows additional configuration options that can be set in
``CMakeLists.txt``

* if the memory is SRAM1, SRAM2, CCD, or AON, then place the full object in the
  sections for example:

  .. code-block:: none

     zephyr_code_relocate(FILES src/file1.c LOCATION SRAM2)
     zephyr_code_relocate(FILES src/file2.c LOCATION SRAM)

* if the memory type is appended with _DATA, _TEXT, _RODATA or _BSS, only the
  selected memory is placed in the required memory region.
  for example:

  .. code-block:: none

     zephyr_code_relocate(FILES src/file1.c LOCATION SRAM2_DATA)
     zephyr_code_relocate(FILES src/file2.c LOCATION SRAM2_TEXT)

* Multiple regions can also be appended together such as: SRAM2_DATA_BSS.
  This will place data and bss inside SRAM2.

* Multiple files can be passed to the FILES argument, or CMake generator
  expressions can be used to relocate a comma-separated list of files

  .. code-block:: none

     file(GLOB sources "file*.c")
     zephyr_code_relocate(FILES ${sources} LOCATION SRAM)
     zephyr_code_relocate(FILES $<TARGET_PROPERTY:my_tgt,SOURCES> LOCATION SRAM)

NOKEEP flag
===========

By default, all relocated functions and variables will be marked with ``KEEP()``
when generating ``linker_relocate.ld``.  Therefore, if any input file happens to
contain unused symbols, then they will not be discarded by the linker, even when
it is invoked with ``--gc-sections``. If you'd like to override this behavior,
you can pass ``NOKEEP`` to your ``zephyr_code_relocate()`` call.

  .. code-block:: none

     zephyr_code_relocate(FILES src/file1.c LOCATION SRAM2_TEXT NOKEEP)

The example above will help ensure that any unused code found in the .text
sections of ``file1.c`` will not stick to SRAM2.

NOCOPY flag
===========

When a ``NOCOPY`` option is passed to the ``zephyr_code_relocate()`` function,
the relocation code is not generated in ``code_relocation.c``. This flag can be
used when we want to move the content of a specific file (or set of files) to a
XIP area.

This example will place the .text section of the ``xip_external_flash.c`` file
to the ``EXTFLASH`` memory region where it will be executed from (XIP). The
.data will be relocated as usual into SRAM.

  .. code-block:: none

     zephyr_code_relocate(FILES src/xip_external_flash.c LOCATION EXTFLASH_TEXT NOCOPY)
     zephyr_code_relocate(FILES src/xip_external_flash.c LOCATION SRAM_DATA)

Relocating libraries
====================

Libraries can be relocated using the LIBRARY argument to
``zephyr_code_relocation()`` with the library name. For example, the following
snippet will relocate serial drivers to SRAM2:

  .. code-block:: none

    zephyr_code_relocate(LIBRARY drivers__serial LOCATION SRAM2)

Tips
====

Take care if relocating kernel/arch files, some contain early initialization
code that executes before code relocation takes place.

Additional MPU/MMU configuration may be required to ensure that the
destination memory region is configured to allow code execution.

Samples/ Tests
==============

A test showcasing this feature is provided at
``$ZEPHYR_BASE/tests/application_development/code_relocation``

This test shows how the code relocation feature is used.

This test will place .text, .data, .bss from 3 files to various parts in the SRAM
using a custom linker file derived from ``include/zephyr/arch/arm/cortex_m/scripts/linker.ld``

A sample showcasing the NOCOPY flag is provided at
``$ZEPHYR_BASE/samples/application_development/code_relocation_nocopy/``
