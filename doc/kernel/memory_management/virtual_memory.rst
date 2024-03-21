.. _memory_management_api_virtual_memory:

Virtual Memory
##############

Virtual memory (VM) in Zephyr provides developers with the ability to fine tune
access to memory. To utilize virtual memory, the platform must support
Memory Management Unit (MMU) and it must be enabled in the build. Due to
the target of Zephyr mainly being embedded systems, virtual memory
support in Zephyr differs a bit from that in traditional operating
systems:

Mapping of Kernel Image
  Default is to do 1:1 mapping for the kernel image (including code and data)
  between physical and virtual memory address spaces, if demand paging
  is not enabled. Deviation from this requires careful manipulation of
  linker script.

Secondary Storage
  Basic virtual memory support does not utilize secondary storage to
  extend usable memory. The maximum usable memory is the same as
  the physical memory.

  * :ref:`memory_management_api_demand_paging` enables utilizing
    secondary storage as a backing store for virtual memory, thus
    allowing larger usable memory than the available physical memory.
    Note that demand paging needs to be explicitly enabled.

  * Although the virtual memory space can be larger than physical
    memory space, without enabling demand paging, all virtually
    mapped memory must be backed by physical memory.


Kconfigs
********

Required
========

These are the Kconfigs that need to be enabled or defined for kernel to support
virtual memory.

* :kconfig:option:`CONFIG_MMU`: must be enabled for virtual memory support in
  kernel.

* :kconfig:option:`CONFIG_MMU_PAGE_SIZE`: size of a memory page. Default is 4KB.

* :kconfig:option:`CONFIG_KERNEL_VM_BASE`: base address of virtual address space.

* :kconfig:option:`CONFIG_KERNEL_VM_SIZE`: size of virtual address space.
  Default is 8MB.

* :kconfig:option:`CONFIG_KERNEL_VM_OFFSET`: kernel image starts at this offset
  from :kconfig:option:`CONFIG_KERNEL_VM_BASE`.

Optional
========

* :kconfig:option:`CONFIG_KERNEL_DIRECT_MAP`: permits 1:1 mappings between
  virtual and physical addresses, instead of kernel choosing addresses within
  the virtual address space. This is useful for mapping device MMIO regions for
  more precise access control.


Memory Map Overview
*******************

This is an overview of the memory map of the virtual memory address space.
Note that the ``Z_*`` macros, which are used in code, may have different
meanings depending on architecture and Kconfigs, which will be explained
below.

.. code-block:: none
   :emphasize-lines: 1, 3, 9, 22, 24

   +--------------+ <- Z_VIRT_RAM_START
   | Undefined VM | <- architecture specific reserved area
   +--------------+ <- Z_KERNEL_VIRT_START
   | Mapping for  |
   | main kernel  |
   | image        |
   |              |
   |              |
   +--------------+ <- Z_FREE_VM_START
   |              |
   | Unused,      |
   | Available VM |
   |              |
   |..............| <- grows downward as more mappings are made
   | Mapping      |
   +--------------+
   | Mapping      |
   +--------------+
   | ...          |
   +--------------+
   | Mapping      |
   +--------------+ <- memory mappings start here
   | Reserved     | <- special purpose virtual page(s) of size Z_VM_RESERVED
   +--------------+ <- Z_VIRT_RAM_END

* ``Z_VIRT_RAM_START`` is the beginning of the virtual memory address space.
  This needs to be page aligned. Currently, it is the same as
  :kconfig:option:`CONFIG_KERNEL_VM_BASE`.

* ``Z_VIRT_RAM_SIZE`` is the size of the virtual memory address space.
  This needs to be page aligned. Currently, it is the same as
  :kconfig:option:`CONFIG_KERNEL_VM_SIZE`.

* ``Z_VIRT_RAM_END`` is simply (``Z_VIRT_RAM_START`` + ``Z_VIRT_RAM_SIZE``).

* ``Z_KERNEL_VIRT_START`` is the same as ``z_mapped_start`` specified in the linker
  script. This is the virtual address of the beginning of the kernel image at
  boot time.

* ``Z_KERNEL_VIRT_END`` is the same as ``z_mapped_end`` specified in the linker
  script. This is the virtual address of the end of the kernel image at boot time.

* ``Z_FREE_VM_START`` is the beginning of the virtual address space where addresses
  can be allocated for memory mapping. This depends on whether
  :kconfig:option:`CONFIG_ARCH_MAPS_ALL_RAM` is enabled.

  * If it is enabled, which means all physical memory are mapped in virtual
    memory address space, and it is the same as
    (:kconfig:option:`CONFIG_SRAM_BASE_ADDRESS` + :kconfig:option:`CONFIG_SRAM_SIZE`).

  * If it is disabled, ``Z_FREE_VM_START`` is the same ``Z_KERNEL_VIRT_END`` which
    is the end of the kernel image.

* ``Z_VM_RESERVED`` is an area reserved to support kernel functions. For example,
  some addresses are reserved to support demand paging.


Virtual Memory Mappings
***********************

Setting up Mappings at Boot
===========================

In general, most supported architectures set up the memory mappings at boot as
following:

* ``.text`` section is read-only and executable. It is accessible in
  both kernel and user modes.

* ``.rodata`` section is read-only and non-executable. It is accessible
  in both kernel and user modes.

* Other kernel sections, such as ``.data``, ``.bss`` and ``.noinit``, are
  read-write and non-executable. They are only accessible in kernel mode.

  * Stacks for user mode threads are automatically granted read-write access
    to their corresponding user mode threads during thread creation.

  * Global variables, by default, are not accessible to user mode threads.
    Refer to :ref:`Memory Domains and Partitions<memory_domain>` on how to
    use global variables in user mode threads, and on how to share data
    between user mode threads.

Caching modes for these mappings are architecture specific. They can be
none, write-back, or write-through.

Note that SoCs have their own additional mappings required to boot where
these mappings are defined under their own SoC configurations. These mappings
usually include device MMIO regions needed to setup the hardware.


Mapping Anonymous Memory
========================

The unused physical memory can be mapped in virtual address space on demand.
This is conceptually similar to memory allocation from heap, but these
mappings must be aligned on page size and have finer access control.

* :c:func:`k_mem_map` can be used to map unused physical memory:

  * The requested size must be multiple of page size.

  * The address returned is inside the virtual address space between
    ``Z_FREE_VM_START`` and ``Z_VIRT_RAM_END``.

  * The mapped region is not guaranteed to be physically contiguous in memory.

  * Guard pages immediately before and after the mapped virtual region are
    automatically allocated to catch access issue due to buffer underrun
    or overrun.

* The mapped region can be unmapped (i.e. freed) via :c:func:`k_mem_unmap`:

  * Caution must be exercised to give the pass the same region size to
    both :c:func:`k_mem_map` and :c:func:`k_mem_unmap`. The unmapping
    function does not check if it is a valid mapped region before unmapping.


API Reference
*************

.. doxygengroup:: kernel_memory_management
