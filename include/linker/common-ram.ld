/* SPDX-License-Identifier: Apache-2.0 */

	SECTION_DATA_PROLOGUE(initlevel,,)
	{
		DEVICE_INIT_SECTIONS()
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined(CONFIG_GEN_ISR_TABLES) && defined(CONFIG_DYNAMIC_INTERRUPTS)
	SECTION_DATA_PROLOGUE(sw_isr_table,,)
	{
		/*
		 * Some arch requires an entry to be aligned to arch
		 * specific boundary for using double word load
		 * instruction.  See include/sw_isr_table.h.
		 */
		. = ALIGN(CONFIG_ARCH_SW_ISR_TABLE_ALIGN);
		*(SW_ISR_TABLE)
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

	/* verify we don't have rogue .init_<something> initlevel sections */
	SECTION_DATA_PROLOGUE(initlevel_error,,)
	{
		DEVICE_INIT_UNDEFINED_SECTION()
	}
	ASSERT(SIZEOF(initlevel_error) == 0, "Undefined initialization levels used.")

	SECTION_DATA_PROLOGUE(initshell,,)
	{
		SHELL_INIT_SECTIONS()
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(log_dynamic_sections,,)
	{
		__log_dynamic_start = .;
		KEEP(*(SORT(.log_dynamic_*)));
		__log_dynamic_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_static_thread_area,,SUBALIGN(4))
	{
		__static_thread_data_list_start = .;
		KEEP(*(SORT_BY_NAME(".__static_thread_data.static.*")))
		__static_thread_data_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef CONFIG_USERSPACE
	/* All kernel objects within are assumed to be either completely
	 * initialized at build time, or initialized automatically at runtime
	 * via iteration before the POST_KERNEL phase.
	 *
	 * These two symbols only used by gen_kobject_list.py
	 */

	_static_kernel_objects_begin = .;
#endif /* CONFIG_USERSPACE */

	SECTION_DATA_PROLOGUE(_k_timer_area,,SUBALIGN(4))
	{
		_k_timer_list_start = .;
		KEEP(*("._k_timer.static.*"))
		_k_timer_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_mem_slab_area,,SUBALIGN(4))
	{
		_k_mem_slab_list_start = .;
		KEEP(*("._k_mem_slab.static.*"))
		_k_mem_slab_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_mem_pool_area,,SUBALIGN(4))
	{
		_k_mem_pool_list_start = .;
		KEEP(*("._k_mem_pool.static.*"))
		_k_mem_pool_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_sem_area,,SUBALIGN(4))
	{
		_k_sem_list_start = .;
		KEEP(*("._k_sem.static.*"))
		KEEP(*("._sys_sem.static.*"))
		_k_sem_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_mutex_area,,SUBALIGN(4))
	{
		_k_mutex_list_start = .;
		KEEP(*("._k_mutex.static.*"))
		_k_mutex_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_queue_area,,SUBALIGN(4))
	{
		_k_queue_list_start = .;
		KEEP(*("._k_queue.static.*"))
		KEEP(*("._k_fifo.static.*"))
		KEEP(*("._k_lifo.static.*"))
		_k_queue_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_stack_area,,SUBALIGN(4))
	{
		_k_stack_list_start = .;
		KEEP(*("._k_stack.static.*"))
		_k_stack_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_msgq_area,,SUBALIGN(4))
	{
		_k_msgq_list_start = .;
		KEEP(*("._k_msgq.static.*"))
		_k_msgq_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_mbox_area,,SUBALIGN(4))
	{
		_k_mbox_list_start = .;
		KEEP(*("._k_mbox.static.*"))
		_k_mbox_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_k_pipe_area,,SUBALIGN(4))
	{
		_k_pipe_list_start = .;
		KEEP(*("._k_pipe.static.*"))
		_k_pipe_list_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(_net_buf_pool_area,,SUBALIGN(4))
	{
		_net_buf_pool_list = .;
		KEEP(*(SORT_BY_NAME("._net_buf_pool.static.*")))
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(net_if,,SUBALIGN(4))
	{
		__net_if_start = .;
		*(".net_if.*")
		KEEP(*(SORT_BY_NAME(".net_if.*")))
		__net_if_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(net_if_dev,,SUBALIGN(4))
	{
		__net_if_dev_start = .;
		*(".net_if_dev.*")
		KEEP(*(SORT_BY_NAME(".net_if_dev.*")))
		__net_if_dev_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined(CONFIG_NET_SHELL)
	SECTION_DATA_PROLOGUE(net_stack,,SUBALIGN(4))
	{
		__net_stack_start = .;
		*(".net_stack.*")
		KEEP(*(SORT_BY_NAME(".net_stack*")))
		__net_stack_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_NET_SHELL */

	SECTION_DATA_PROLOGUE(net_l2_data,,SUBALIGN(4))
	{
		__net_l2_data_start = .;
		*(".net_l2.data")
		KEEP(*(SORT_BY_NAME(".net_l2.data*")))
		__net_l2_data_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

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
