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

.. note::
   The LLEXT instruction heap is grouped with Zephyr .rodata, which the linker
   typically places after .text in instruction memory.

.. warning::
   LLEXT will be unable to link and execute extensions if instruction memory
   (i.e., memory the processor can fetch instructions from) is not writable.

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
