/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

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

	ITERABLE_SECTION_ROM_NUMERIC(device, Z_LINK_ITERABLE_SUBALIGN)

#if defined(CONFIG_GEN_SW_ISR_TABLE) && defined(CONFIG_SHARED_INTERRUPTS)
	/* since z_shared_isr() is not referenced anywhere when
	 * zephyr_pre0.elf is built, the linker will end up dropping it.
	 * Later on, during the second linking stage (when zephyr.elf is
	 * built), the symbol will be added to the text section since it's
	 * now being referenced (thanks to isr_tables.c). This is very
	 * problematic because adding the z_shared_isr symbol between
	 * the linking stages will end up shifting the addresses of the
	 * functions, which, in turn, will end up messing the ISR table
	 * (as the entries from _sw_isr_table will end up pointing to
	 * old addresses of the registered ISRs). To prevent this from
	 * happening, instruct the linker to avoid dropping z_shared_isr
	 * if it's not being referenced anywhere.
	 */
	SECTION_PROLOGUE(.text.z_shared_isr,,)
	{
		KEEP(*(.text.z_shared_isr*))
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

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

#if defined(CONFIG_SHARED_INTERRUPTS)
	SECTION_PROLOGUE(shared_sw_isr_table,,)
	{
		/* TODO: does this section require alignment? */
		KEEP(*(_SHARED_SW_ISR_TABLE_SECTION_SYMS))
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

#endif

	/* verify we don't have rogue .z_init_<something> initlevel sections */
	SECTION_PROLOGUE(initlevel_error,,)
	{
		KEEP(*(SORT(.z_init_*)))
	}
	ASSERT(SIZEOF(initlevel_error) == 0, "Undefined initialization levels used.")

#ifdef CONFIG_USERSPACE
	/* Build-time assignment of permissions to kernel objects to
	 * threads declared with K_THREAD_DEFINE()
	 */
	ITERABLE_SECTION_ROM(k_object_assignment, Z_LINK_ITERABLE_SUBALIGN)
#endif

	SECTION_DATA_PROLOGUE(app_shmem_regions,,)
	{
		__app_shmem_regions_start = .;
		KEEP(*(SORT(.app_regions.*)));
		__app_shmem_regions_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	ITERABLE_SECTION_ROM(k_p4wq_initparam, Z_LINK_ITERABLE_SUBALIGN)

	ITERABLE_SECTION_ROM(_static_thread_data, Z_LINK_ITERABLE_SUBALIGN)

#if defined(CONFIG_PCIE)
	ITERABLE_SECTION_ROM(irq_alloc, Z_LINK_ITERABLE_SUBALIGN)
#endif /* CONFIG_PCIE */

#if !defined(CONFIG_DEVICE_DEPS_DYNAMIC)
	SECTION_DATA_PROLOGUE(device_deps,,)
	{
#include <zephyr/linker/device-deps.ld>
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* !CONFIG_DEVICE_DEPS_DYNAMIC */

#include <device-api-sections.ld>
