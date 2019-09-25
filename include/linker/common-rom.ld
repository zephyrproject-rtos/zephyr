/* SPDX-License-Identifier: Apache-2.0 */

#if defined(CONFIG_GEN_ISR_TABLES) && !defined(CONFIG_DYNAMIC_INTERRUPTS)
	SECTION_PROLOGUE(sw_isr_table,,)
	{
		/*
		 * Some arch requires an entry to be aligned to arch
		 * specific boundary for using double word load
		 * instruction.  See include/sw_isr_table.h.
		 */
		. = ALIGN(CONFIG_ARCH_SW_ISR_TABLE_ALIGN);
		*(SW_ISR_TABLE)
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif
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
	SECTION_PROLOGUE(object_access,,)
	{
		__k_object_assignment_list_start = .;
		KEEP(*(".__k_object_assignment.*"))
		__k_object_assignment_list_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

	SECTION_PROLOGUE(app_shmem_regions,,)
	{
		__app_shmem_regions_start = .;
		KEEP(*(SORT(".app_regions.*")));
		__app_shmem_regions_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE (devconfig,,)
	{
		__devconfig_start = .;
		*(".devconfig.*")
		KEEP(*(SORT_BY_NAME(".devconfig*")))
		__devconfig_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(net_l2,,)
	{
		__net_l2_start = .;
		*(".net_l2.init")
		KEEP(*(SORT_BY_NAME(".net_l2.init*")))
		__net_l2_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

#if defined(CONFIG_NET_SOCKETS)
	SECTION_PROLOGUE(net_socket_register,,)
	{
		_net_socket_register_list_start = .;
		KEEP(*(SORT_BY_NAME("._net_socket_register.*")))
		_net_socket_register_list_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

#if defined(CONFIG_NET_L2_PPP)
	SECTION_PROLOGUE(net_ppp_proto,,)
	{
		__net_ppp_proto_start = .;
		*(".net_ppp_proto.*")
		KEEP(*(SORT_BY_NAME(".net_ppp_proto.*")))
		__net_ppp_proto_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

	SECTION_DATA_PROLOGUE(_bt_channels_area,,SUBALIGN(4))
	{
		_bt_l2cap_fixed_chan_list_start = .;
		KEEP(*(SORT_BY_NAME("._bt_l2cap_fixed_chan.static.*")))
		_bt_l2cap_fixed_chan_list_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

#if defined(CONFIG_BT_BREDR)
	SECTION_DATA_PROLOGUE(_bt_br_channels_area,,SUBALIGN(4))
	{
		_bt_l2cap_br_fixed_chan_list_start = .;
		KEEP(*(SORT_BY_NAME("._bt_l2cap_br_fixed_chan.static.*")))
		_bt_l2cap_br_fixed_chan_list_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

	SECTION_DATA_PROLOGUE(_bt_services_area,,SUBALIGN(4))
	{
		_bt_gatt_service_static_list_start = .;
		KEEP(*(SORT_BY_NAME("._bt_gatt_service_static.static.*")))
		_bt_gatt_service_static_list_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

#if defined(CONFIG_SETTINGS)
	SECTION_DATA_PROLOGUE(_settings_handlers_area,,SUBALIGN(4))
	{
		_settings_handler_static_list_start = .;
		KEEP(*(SORT_BY_NAME("._settings_handler_static.static.*")))
		_settings_handler_static_list_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

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
