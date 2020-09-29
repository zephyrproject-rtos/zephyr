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
		CREATE_OBJ_LEVEL(init, PRE_KERNEL_1)
		CREATE_OBJ_LEVEL(init, PRE_KERNEL_2)
		CREATE_OBJ_LEVEL(init, POST_KERNEL)
		CREATE_OBJ_LEVEL(init, APPLICATION)
		CREATE_OBJ_LEVEL(init, SMP)
		__init_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

#if defined(CONFIG_GEN_SW_ISR_TABLE) && !defined(CONFIG_DYNAMIC_INTERRUPTS)
	SECTION_PROLOGUE(sw_isr_table,,)
	{
		/*
		 * Some arch requires an entry to be aligned to arch
		 * specific boundary for using double word load
		 * instruction.  See include/sw_isr_table.h.
		 */
		. = ALIGN(CONFIG_ARCH_SW_ISR_TABLE_ALIGN);
		*(_SW_ISR_TABLE_SECTION_NAME)
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

	/* verify we don't have rogue .init_<something> initlevel sections */
	SECTION_PROLOGUE(initlevel_error,,)
	{
		KEEP(*(SORT(.init_[_A-Z0-9]*)))
	}
	ASSERT(SIZEOF(initlevel_error) == 0, "Undefined initialization levels used.")

#ifdef CONFIG_CPLUSPLUS
	SECTION_PROLOGUE(_CTOR_SECTION_NAME,,)
	{
		/*
		 * The compiler fills the constructor pointers table below,
		 * hence symbol __CTOR_LIST__ must be aligned on word
		 * boundary.  To align with the C++ standard, the first elment
		 * of the array contains the number of actual constructors. The
		 * last element is NULL.
		 */
#ifdef CONFIG_64BIT
		. = ALIGN(8);
		__CTOR_LIST__ = .;
		QUAD((__CTOR_END__ - __CTOR_LIST__) / 8 - 2)
		KEEP(*(SORT_BY_NAME(".ctors*")))
		QUAD(0)
		__CTOR_END__ = .;
#else
		. = ALIGN(4);
		__CTOR_LIST__ = .;
		LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
		KEEP(*(SORT_BY_NAME(".ctors*")))
		LONG(0)
		__CTOR_END__ = .;
#endif
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(init_array,,)
	{
		. = ALIGN(4);
		__init_array_start = .;
		KEEP(*(SORT_BY_NAME(".init_array*")))
		__init_array_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif
#ifdef CONFIG_USERSPACE
	/* Build-time assignment of permissions to kernel objects to
	 * threads declared with K_THREAD_DEFINE()
	 */
	Z_ITERABLE_SECTION_ROM(z_object_assignment, 4)
#endif

	SECTION_PROLOGUE(app_shmem_regions,,)
	{
		__app_shmem_regions_start = .;
		KEEP(*(SORT(.app_regions.*)));
		__app_shmem_regions_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

#if defined(CONFIG_NET_SOCKETS)
	Z_ITERABLE_SECTION_ROM(net_socket_register, 4)
#endif

#if defined(CONFIG_NET_L2_PPP)
	Z_ITERABLE_SECTION_ROM(ppp_protocol_handler, 4)
#endif

	Z_ITERABLE_SECTION_ROM(bt_l2cap_fixed_chan, 4)

#if defined(CONFIG_BT_BREDR)
	Z_ITERABLE_SECTION_ROM(bt_l2cap_br_fixed_chan, 4)
#endif

	Z_ITERABLE_SECTION_ROM(bt_gatt_service_static, 4)

#if defined(CONFIG_EC_HOST_CMD)
	Z_ITERABLE_SECTION_ROM(ec_host_cmd_handler, 4)
#endif

#if defined(CONFIG_SETTINGS)
	Z_ITERABLE_SECTION_ROM(settings_handler_static, 4)
#endif

#if defined(CONFIG_EMUL)
	SECTION_DATA_PROLOGUE(log_const_sections,,)
	{
		__emul_list_start = .;
		KEEP(*(SORT_BY_NAME(".emulators")));
		__emul_list_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif /* CONFIG_EMUL */

	SECTION_DATA_PROLOGUE(log_const_sections,,)
	{
		__log_const_start = .;
		KEEP(*(SORT(.log_const_*)));
		__log_const_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(log_backends_sections,,)
	{
		__log_backends_start = .;
		KEEP(*("._log_backend.*"));
		__log_backends_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	Z_ITERABLE_SECTION_ROM(shell, 4)

	SECTION_DATA_PROLOGUE(shell_root_cmds_sections,,)
	{
		__shell_root_cmds_start = .;
		KEEP(*(SORT(.shell_root_cmd_*)));
		__shell_root_cmds_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(font_entry_sections,,)
	{
		__font_entry_start = .;
		KEEP(*(SORT_BY_NAME("._cfb_font.*")))
		__font_entry_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	Z_ITERABLE_SECTION_ROM(tracing_backend, 4)
