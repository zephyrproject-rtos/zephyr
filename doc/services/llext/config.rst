Configuration
#############

The following Kconfig options are available for the LLEXT subsystem:

.. _llext_kconfig_heap:

Heap size
----------

The LLEXT subsystem needs a heap to be allocated for extension related data.
The following option controls this allocation, when allocating a static heap.

:kconfig:option:`CONFIG_LLEXT_HEAP_SIZE`

        Size of the LLEXT heap in kilobytes.

For boards using the Harvard architecture, the LLEXT heap is split into two:
one heap in instruction memory and another in data memory. The following options
control these allocations.

:kconfig:option:`CONFIG_LLEXT_INSTR_HEAP_SIZE`

        Size of the LLEXT heap in instruction memory in kilobytes.

:kconfig:option:`CONFIG_LLEXT_DATA_HEAP_SIZE`

        Size of the LLEXT heap in data memory in kilobytes.

Alternatively the application can configure a dynamic heap using the following
option.

:kconfig:option:`CONFIG_LLEXT_HEAP_DYNAMIC`

        Some applications require loading extensions into the memory which does
        not exist during the boot time and cannot be allocated statically. Make
        the application responsible for LLEXT heap allocation. Do not allocate
        LLEXT heap statically.

        Application must call :c:func:`llext_heap_init` in order to assign a
        buffer to be used as the LLEXT heap, otherwise LLEXT modules will not
        load. When the application does not need LLEXT functionality any more,
        it should call :c:func:`llext_heap_uninit` which releases control of
        the buffer back to the application.

.. note::

   When :ref:`user mode <usermode_api>` is enabled, the heap size must be
   large enough to allow the extension sections to be allocated with the
   alignment required by the architecture.

.. note::

   On Harvard architectures, applications must call
   :c:func:`llext_heap_init_harvard`.

The backing data structure for the LLEXT heap is by default a
:c:struct:`k_heap`, but it can be changed to :c:type:`sys_mem_blocks_t` for
non-metadata (i.e., extension regions) by selecting
:kconfig:option:`CONFIG_LLEXT_HEAP_MEMBLK` for
:kconfig:option:`CONFIG_LLEXT_HEAP_MANAGEMENT`.

:kconfig:option:`CONFIG_LLEXT_HEAP_MANAGEMENT`

        Select the memory management API used to store extension regions in
        LLEXT heap memory. This choice does not affect LLEXT metadata, which
        is always managed with :c:struct:`k_heap`.

:kconfig:option:`CONFIG_LLEXT_HEAP_MEMBLK`

        Use :c:type:`sys_mem_blocks_t` API to manage LLEXT heap memory for
        extension regions. A minimum of one block will be allocated per region.
        Block size must be selected with care to ensure proper alignment for
        extension regions.

.. note::

   :kconfig:option:`CONFIG_LLEXT_HEAP_MEMBLK` does not support
   :kconfig:option:`CONFIG_LLEXT_HEAP_DYNAMIC`.

The heap will be split into two, with the size of each sub-heap controlled by
the following options.

:kconfig:option:`CONFIG_LLEXT_EXT_HEAP_SIZE`

        Heap size in kilobytes available for LLEXT extension sections. Must be
        a multiple of :kconfig:option:`CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE`.
        Replaced by :kconfig:option:`CONFIG_LLEXT_INSTR_HEAP_SIZE` and
        :kconfig:option:`CONFIG_LLEXT_DATA_HEAP_SIZE` if
        :kconfig:option:`CONFIG_HARVARD` is selected.

:kconfig:option:`CONFIG_LLEXT_METADATA_HEAP_SIZE`

        Heap size in kilobytes available for LLEXT metadata.

Another option controls block size.

:kconfig:option:`CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE`

        Block size in bytes for LLEXT :c:type:`sys_mem_blocks_t` heap(s).
        Must be equal to or a multiple of ``LLEXT_PAGE_SIZE`` if MMU or MPU
        are enabled. The block size must also be equal to or a multiple of the
        largest alignment needed for any extension region. If
        :kconfig:option:`CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT` is
        selected and regions are large, an unreasonably large block size may be
        needed to satisfy alignment requirements.

Heap placement
--------------

The LLEXT heap(s) have custom sections. Non-Harvard heap sections
(``.llext_heap``, or ``.llext_metadata_heap`` and ``.llext_ext_heap`` if
:kconfig:option:`CONFIG_LLEXT_HEAP_MEMBLK` is selected) are placed alongside
``.noinit`` sections in the file
:file:`include/zephyr/linker/common-noinit.ld`. If none of your linker scripts
include this file, you will need to place the non-Harvard LLEXT heap sections
manually. One way to do this is by including :file:`snippets-noinit.ld` in
your linker script after your ``.noinit`` sections.

.. code-block:: none

   /* Located in generated directory. This file is populated by the
    * zephyr_linker_sources() CMake function.
    */
   #include <snippets-noinit.ld>

Add the file as a linker source in your board, SoC, or architecture
:file:`CMakeFiles.txt`.

.. code-block:: cmake

   zephyr_linker_sources(NOINIT snippets-noinit.ld)

Then create a file in the same directory as your :file:`CMakeFiles.txt` named
:file:`noinit.ld`.

.. code-block:: none

   #if defined(CONFIG_LLEXT) && !defined(CONFIG_LLEXT_CUSTOM_HEAP_PLACEMENT)
   *(.llext_heap)
   *(.llext_ext_heap)
   *(.llext_metadata_heap)
   #endif /* CONFIG_LLEXT && !CONFIG_LLEXT_CUSTOM_HEAP_PLACEMENT */

For ARC, the Harvard instruction and data heap sections (``.llext_instr_heap``
and ``.llext_data_heap``) are placed in instruction and data memory at the
architecture level. If you are using a non-ARC board with a Harvard
architecture, you will need to manually place ``.llext_instr_heap`` and
``.llext_data_heap``.

.. warning::

   LLEXT will be unable to load extensions if the instruction memory
   ``.llext_instr_heap`` is placed in is not writable at the time the
   extensions are loaded and linked.

Placements can also be specified by providing a custom linker script.

:kconfig:option:`CONFIG_CUSTOM_LINKER_SCRIPT`

        Path to the linker script to be used instead of the one defined by the
        board.

        The linker script must be based on a version provided by Zephyr since
        the kernel can expect a certain layout/certain regions.

        This is useful when an application needs to add sections into the
        linker script and avoid having to change the script provided by
        Zephyr.

While using a custom linker script, you may need to override default
placements. For example, you may wish to include
:file:`include/zephyr/linker/common-noinit.ld` in your linker script
but place the heap section(s) elsewhere. To do this, select the following
option.

:kconfig:option:`CONFIG_LLEXT_CUSTOM_HEAP_PLACEMENT`

        Remove default placements of LLEXT heap sections in the linker script,
        allowing the user to place the heap(s) themselves.

.. _llext_kconfig_type:

ELF object type
---------------

The LLEXT subsystem supports loading different types of extensions; the type
can be set by choosing among the following Kconfig options:

:kconfig:option:`CONFIG_LLEXT_TYPE_ELF_OBJECT`

        Build and expect relocatable files as binary object type for the LLEXT
        subsystem. A single compiler invocation is used to generate the object
        file.

:kconfig:option:`CONFIG_LLEXT_TYPE_ELF_RELOCATABLE`

        Build and expect relocatable (partially linked) files as the binary
        object type for the LLEXT subsystem. These object files are generated
        by the linker by combining multiple object files into a single one.

:kconfig:option:`CONFIG_LLEXT_TYPE_ELF_SHAREDLIB`

        Build and expect shared libraries as binary object type for the LLEXT
        subsystem. The standard linking process is used to generate the shared
        library from multiple object files.

        .. note::

           This is not currently supported on ARM architectures.

.. _llext_kconfig_storage:

Minimize allocations
--------------------

The LLEXT subsystem loading mechanism, by default, uses a seek/read abstraction
and copies all data into allocated memory; this is done to allow the extension
to be loaded from any storage medium. Sometimes, however, data is already in a
buffer in RAM and copying it is not necessary. The following option allows the
LLEXT subsystem to optimize memory footprint in this case.

:kconfig:option:`CONFIG_LLEXT_STORAGE_WRITABLE`

        Allow the extension to be loaded by directly referencing section data
        into the ELF buffer. To be effective, this requires the use of an ELF
        loader that supports the ``peek`` functionality, such as the
        :c:struct:`llext_buf_loader`.

        .. warning::

           The application must ensure that the buffer used to load the
           extension remains allocated until the extension is unloaded.

        .. note::

           This will directly modify the contents of the buffer during the link
           phase. Once the extension is unloaded, the buffer must be reloaded
           before it can be used again in a call to :c:func:`llext_load`.

        .. note::

           This is currently required by the Xtensa architecture. Further
           information on this topic is available on GitHub issue `#75341
           <https://github.com/zephyrproject-rtos/zephyr/issues/75341>`_.

.. _llext_symbol_groups:

Symbol Groups
-------------

All LLEXT symbols belong to a group, with the inclusion of each group in the
exported symbol table controlled by a corresponding Kconfig symbol. Exporting
a symbol as part of a group is done with the :c:macro:`EXPORT_GROUP_SYMBOL`
and :c:macro:`EXPORT_GROUP_SYMBOL_NAMED` macros. For example the following
exports the symbol ``memcpy`` as part of the ``LIBC`` group:

.. code:: c

   EXPORT_GROUP_SYMBOL(LIBC, memcpy);

Group names are arbitrary, but they must be all uppercase. For each group
used in C code, there **MUST** be a corresponding Kconfig symbol of the form:

.. code::

   config LLEXT_EXPORT_SYMBOL_GROUP_{GROUP_NAME}
      bool "Export all symbols from the {GROUP_NAME} group"

The default group for symbols (those declared with :c:macro:`EXPORT_SYMBOL`
or :c:macro:`EXPORT_SYMBOL_NAMED`) is the ``UNASSIGNED`` group. As per the
above rules, the inclusion of this group is controlled by
:kconfig:option:`CONFIG_LLEXT_EXPORT_SYMBOL_GROUP_UNASSIGNED`.

The groups currently defined by Zephyr are:

.. csv-table:: Zephyr LLEXT symbol groups
  :header: Group Name, Kconfig Symbol, Description

  ``UNASSIGNED``, :kconfig:option:`CONFIG_LLEXT_EXPORT_SYMBOL_GROUP_UNASSIGNED`, Symbols without an explicit group
  ``SYSCALL``, :kconfig:option:`CONFIG_LLEXT_EXPORT_SYMBOL_GROUP_SYSCALL`, Zephyr kernel system calls
  ``LIBC``, :kconfig:option:`CONFIG_LLEXT_EXPORT_SYMBOL_GROUP_LIBC`, C standard library functions (:c:func:`memcpy` etc)
  ``DEVICE``, :kconfig:option:`CONFIG_LLEXT_EXPORT_SYMBOL_GROUP_DEVICE`, Devicetree devices

.. _llext_kconfig_slid:

Using SLID for symbol lookups
-----------------------------

When an extension is loaded, the LLEXT subsystem must find the address of all
the symbols residing in the main application that the extension references.
To this end, the main binary contains a LLEXT-dedicated symbol table, filled
with one symbol-name-to-address mapping entry for each symbol exported by the
main application to extensions. This table can then be searched into by the
LLEXT linker at extension load time. This process is pretty slow due to the
nature of string comparisons, and the size consumed by the table can become
significant as the number of exported symbols increases.

:kconfig:option:`CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID`

        Perform an extra processing step on the Zephyr binary and on all
        extensions being built, converting every string in the symbol tables to
        a pointer-sized hash called Symbol Link Identifier (SLID), which is
        stored in the binary.

        This speeds up the symbol lookup process by allowing usage of
        integer-based comparisons rather than string-based ones. Another
        benefit of SLID-based linking is that storing symbol names in the
        binary is no longer necessary, which provides a significant decrease in
        symbol table size.

        .. note::

           This option is not currently compatible with the :ref:`LLEXT EDK
           <llext_build_edk>`.

        .. note::

           Using a different value for this option in the main binary and in
           extensions is not supported. For example, if the main application
           is built with ``CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID=y``, it is
           forbidden to load an extension that was compiled with
           ``CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID=n``.

EDK configuration
-----------------

Options influencing the generation and behavior of the LLEXT EDK are described
in :ref:`llext_kconfig_edk`.
