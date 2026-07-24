.. _mem_mgmt_virt_mem_api:

Virtual Memory Backend
######################

Virtual memory backend provides the APIs to support
:ref:`virtual memory<memory_management_api_virtual_memory>` in kernel.
This backend can also be used without kernel virtual memory support, and
thus allowing applications to explicitly control memory mappings.
There can only be one backend enabled per build and the information is
available via :kconfig:option:`CONFIG_MEM_MGMT_VM_BACKEND`.

Here are the APIs for memory mapping:

  * :c:func:`sys_mm_vm_backend_mem_map`
  * :c:func:`sys_mm_vm_backend_mem_unmap`
  * :c:func:`sys_mm_vm_backend_page_phys_get`

Here are the APIs for demand paging (if enabled):

  * :c:func:`sys_mm_vm_backend_reserved_pages_update`
  * :c:func:`sys_mm_vm_backend_mem_page_out`
  * :c:func:`sys_mm_vm_backend_mem_page_in`
  * :c:func:`sys_mm_vm_backend_mem_scratch`
  * :c:func:`sys_mm_vm_backend_page_location_get`
  * :c:func:`sys_mm_vm_backend_mem_page_info_get`

Architecture-specific Virtual Memory Backend
********************************************

This uses the memory mapping code implemented in the architecture layer and
is selected via :kconfig:option:`CONFIG_MEM_MGMT_VM_BACKEND_ARCH`.
Refer to the :ref:`architecture_porting_guide` on specifics. The system
virtual memory backend APIs are simply wrappers to the ones in
the architecture layer:

  * :c:func:`sys_mm_vm_backend_mem_map` -> :c:func:`arch_mem_map`
  * :c:func:`sys_mm_vm_backend_mem_unmap` -> :c:func:`arch_mem_unmap`
  * :c:func:`sys_mm_vm_backend_page_phys_get` -> :c:func:`arch_page_phys_get`
  * :c:func:`sys_mm_vm_backend_reserved_pages_update` -> :c:func:`arch_reserved_pages_update`
  * :c:func:`sys_mm_vm_backend_mem_page_out` -> :c:func:`arch_mem_page_out`
  * :c:func:`sys_mm_vm_backend_mem_page_in` -> :c:func:`arch_mem_page_in`
  * :c:func:`sys_mm_vm_backend_mem_scratch` -> :c:func:`arch_mem_scratch`
  * :c:func:`sys_mm_vm_backend_page_location_get` -> :c:func:`arch_page_location_get`
  * :c:func:`sys_mm_vm_backend_mem_page_info_get` -> :c:func:`arch_page_info_get`

Custom Virtual Memory Backend
*****************************

When :kconfig:option:`CONFIG_MEM_MGMT_VM_BACKEND_CUSTOM` is enabled, a custom
implementation of virtual memory backend will be used. This allows hardware
external to the CPU to provide support for memory mapping. This custom
implementation must implement these APIs to support memory mappings:

  * :c:func:`sys_mm_vm_backend_mem_map`
  * :c:func:`sys_mm_vm_backend_mem_unmap`
  * :c:func:`sys_mm_vm_backend_page_phys_get`

If support for demand paging is needed, these macros must be defined in
header file ``sys_mm_vm_backend_custom.h``:

  * :c:macro:`SYS_MM_VM_DATA_PAGE_LOADED`
  * :c:macro:`SYS_MM_VM_DATA_PAGE_ACCESSED`
  * :c:macro:`SYS_MM_VM_DATA_PAGE_DIRTY`
  * :c:macro:`SYS_MM_VM_DATA_PAGE_NOT_MAPPED`

API Reference
*************

.. doxygengroup:: mem_mgmt_vm_backend
