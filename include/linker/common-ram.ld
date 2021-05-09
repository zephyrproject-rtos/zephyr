/* SPDX-License-Identifier: Apache-2.0 */

#if defined(CONFIG_GEN_SW_ISR_TABLE) && defined(CONFIG_DYNAMIC_INTERRUPTS)
	SECTION_DATA_PROLOGUE(sw_isr_table,,)
	{
		/*
		 * Some arch requires an entry to be aligned to arch
		 * specific boundary for using double word load
		 * instruction.  See include/sw_isr_table.h.
		 */
		. = ALIGN(CONFIG_ARCH_SW_ISR_TABLE_ALIGN);
		*(_SW_ISR_TABLE_SECTION_SYMS)
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

	SECTION_DATA_PROLOGUE(devices,,)
	{
		/* link in devices objects, which are tied to the init ones;
		 * the objects are thus sorted the same way as their init
		 * object parent see include/device.h
		 */
		__device_start = .;
		CREATE_OBJ_LEVEL(device, PRE_KERNEL_1)
		CREATE_OBJ_LEVEL(device, PRE_KERNEL_2)
		CREATE_OBJ_LEVEL(device, POST_KERNEL)
		CREATE_OBJ_LEVEL(device, APPLICATION)
		CREATE_OBJ_LEVEL(device, SMP)
		__device_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(initshell,,)
	{
		/* link in shell initialization objects for all modules that
		 * use shell and their shell commands are automatically
		 * initialized by the kernel.
		 */
		__shell_module_start = .;
		KEEP(*(".shell_module_*"));
		__shell_module_end = .;
		__shell_cmd_start = .;
		KEEP(*(".shell_cmd_*"));
		__shell_cmd_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(log_dynamic_sections,,)
	{
		__log_dynamic_start = .;
		KEEP(*(SORT(.log_dynamic_*)));
		__log_dynamic_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	Z_ITERABLE_SECTION_RAM(_static_thread_data, 4)

#ifdef CONFIG_USERSPACE
	/* All kernel objects within are assumed to be either completely
	 * initialized at build time, or initialized automatically at runtime
	 * via iteration before the POST_KERNEL phase.
	 *
	 * These two symbols only used by gen_kobject_list.py
	 */

	_static_kernel_objects_begin = .;
#endif /* CONFIG_USERSPACE */

	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_timer, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_mem_slab, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_mem_pool, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_heap, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_mutex, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_stack, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_msgq, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_mbox, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_pipe, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_sem, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_queue, 4)
	Z_ITERABLE_SECTION_RAM_GC_ALLOWED(k_condvar, 4)

	SECTION_DATA_PROLOGUE(_net_buf_pool_area,,SUBALIGN(4))
	{
		_net_buf_pool_list = .;
		KEEP(*(SORT_BY_NAME("._net_buf_pool.static.*")))
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined(CONFIG_NETWORKING)
	Z_ITERABLE_SECTION_RAM(net_if, 4)
	Z_ITERABLE_SECTION_RAM(net_if_dev, 4)
	Z_ITERABLE_SECTION_RAM(net_l2, 4)
#endif /* NETWORKING */

#if defined(CONFIG_UART_MUX)
	SECTION_DATA_PROLOGUE(uart_mux,,SUBALIGN(4))
	{
		__uart_mux_start = .;
		*(".uart_mux.*")
		KEEP(*(SORT_BY_NAME(".uart_mux.*")))
		__uart_mux_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

#if defined(CONFIG_USB_DEVICE_STACK)
	SECTION_DATA_PROLOGUE(usb_descriptor,,SUBALIGN(1))
	{
		__usb_descriptor_start = .;
		*(".usb.descriptor")
		KEEP(*(SORT_BY_NAME(".usb.descriptor*")))
		__usb_descriptor_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(usb_data,,SUBALIGN(1))
	{
		__usb_data_start = .;
		*(".usb.data")
		KEEP(*(SORT_BY_NAME(".usb.data*")))
		__usb_data_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_USB_DEVICE_STACK */

#if defined(CONFIG_USB_DEVICE_BOS)
	SECTION_DATA_PROLOGUE(usb_bos_desc,,SUBALIGN(1))
	{
		__usb_bos_desc_start = .;
		*(".usb.bos_desc")
		KEEP(*(SORT_BY_NAME(".usb.bos_desc*")))
		__usb_bos_desc_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_USB_DEVICE_BOS */

#ifdef CONFIG_USERSPACE
	_static_kernel_objects_end = .;
#endif
