/* SPDX-License-Identifier: Apache-2.0 */

	SECTION_PROLOGUE(initlevel,,)
	{
		/*
		 * link in initialization objects for all objects that are
		 * automatically initialized by the kernel; the objects are
		 * sorted in the order they will be initialized (i.e. ordered
		 * by level, sorted by priority within a level)
		 */
		__init_start = .;
		CREATE_OBJ_LEVEL(init, EARLY)
		CREATE_OBJ_LEVEL(init, PRE_KERNEL_1)
		CREATE_OBJ_LEVEL(init, PRE_KERNEL_2)
		CREATE_OBJ_LEVEL(init, POST_KERNEL)
		CREATE_OBJ_LEVEL(init, APPLICATION)
		CREATE_OBJ_LEVEL(init, SMP)
		__init_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_PROLOGUE(devices,,)
	{
		/* Link in devices objects, which are tied to the init ones;
		 * the objects are thus sorted the same way as their init
		 * object parent. See above and include/device.h.
		 */
		__device_start = .;
		CREATE_OBJ_LEVEL(device, EARLY)
		CREATE_OBJ_LEVEL(device, PRE_KERNEL_1)
		CREATE_OBJ_LEVEL(device, PRE_KERNEL_2)
		CREATE_OBJ_LEVEL(device, POST_KERNEL)
		CREATE_OBJ_LEVEL(device, APPLICATION)
		CREATE_OBJ_LEVEL(device, SMP)
		__device_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined(CONFIG_GEN_SW_ISR_TABLE) && !defined(CONFIG_DYNAMIC_INTERRUPTS)
	SECTION_PROLOGUE(sw_isr_table,,)
	{
		/*
		 * Some arch requires an entry to be aligned to arch
		 * specific boundary for using double word load
		 * instruction.  See include/sw_isr_table.h.
		 */
		. = ALIGN(CONFIG_ARCH_SW_ISR_TABLE_ALIGN);
		*(_SW_ISR_TABLE_SECTION_SYMS)
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

	/* verify we don't have rogue .z_init_<something> initlevel sections */
	SECTION_PROLOGUE(initlevel_error,,)
	{
		KEEP(*(SORT(.z_init_[_A-Z0-9]*)))
	}
	ASSERT(SIZEOF(initlevel_error) == 0, "Undefined initialization levels used.")

#ifdef CONFIG_USERSPACE
	/* Build-time assignment of permissions to kernel objects to
	 * threads declared with K_THREAD_DEFINE()
	 */
	ITERABLE_SECTION_ROM(z_object_assignment, 4)
#endif

	SECTION_DATA_PROLOGUE(app_shmem_regions,,)
	{
		__app_shmem_regions_start = .;
		KEEP(*(SORT(.app_regions.*)));
		__app_shmem_regions_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	ITERABLE_SECTION_ROM(k_p4wq_initparam, 4)

	ITERABLE_SECTION_ROM(_static_thread_data, 4)

#if defined(CONFIG_PCIE)
	SECTION_DATA_PROLOGUE(irq_alloc,,)
	{
		__irq_alloc_start = .;
		KEEP(*(SORT_BY_NAME("._irq_alloc*")));
		__irq_alloc_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif /* CONFIG_PCIE */

#if !defined(CONFIG_HAS_DYNAMIC_DEVICE_HANDLES)
	SECTION_DATA_PROLOGUE(device_handles,,)
	{
#include <zephyr/linker/device-handles.ld>
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* !CONFIG_HAS_DYNAMIC_DEVICE_HANDLES */
