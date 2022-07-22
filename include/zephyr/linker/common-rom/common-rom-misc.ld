/* SPDX-License-Identifier: Apache-2.0 */

#if defined(CONFIG_EC_HOST_CMD)
	ITERABLE_SECTION_ROM(ec_host_cmd_handler, 4)
#endif

#if defined(CONFIG_SETTINGS)
	ITERABLE_SECTION_ROM(settings_handler_static, 4)
#endif

#if defined(CONFIG_EMUL)
	SECTION_DATA_PROLOGUE(emulators_section,,)
	{
		__emul_list_start = .;
		KEEP(*(SORT_BY_NAME(".emulators")));
		__emul_list_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif /* CONFIG_EMUL */

	SECTION_DATA_PROLOGUE(symbol_to_keep,,)
	{
		__symbol_to_keep_start = .;
		KEEP(*(SORT(.symbol_to_keep*)));
		__symbol_to_keep_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	ITERABLE_SECTION_ROM(shell, 4)

	SECTION_DATA_PROLOGUE(shell_root_cmds_sections,,)
	{
		__shell_root_cmds_start = .;
		KEEP(*(SORT(.shell_root_cmd_*)));
		__shell_root_cmds_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(shell_subcmds_sections,,)
	{
		__shell_subcmds_start = .;
		KEEP(*(SORT(.shell_subcmd_*)));
		__shell_subcmds_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(shell_dynamic_subcmds_sections,,)
	{
		__shell_dynamic_subcmds_start = .;
		KEEP(*(SORT(.shell_dynamic_subcmd_*)));
		__shell_dynamic_subcmds_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(font_entry_sections,,)
	{
		__font_entry_start = .;
		KEEP(*(SORT_BY_NAME("._cfb_font.*")))
		__font_entry_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
