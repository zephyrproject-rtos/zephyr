/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/linker/iterable_sections.h>

#if defined(CONFIG_EC_HOST_CMD)
	ITERABLE_SECTION_ROM(ec_host_cmd_handler, 4)
#endif

#if defined(CONFIG_SETTINGS)
	ITERABLE_SECTION_ROM(settings_handler_static, 4)
#endif

#if defined(CONFIG_SENSING)
	ITERABLE_SECTION_ROM(sensing_sensor_info, 4)
#endif

#if defined(CONFIG_SENSOR_INFO)
	ITERABLE_SECTION_ROM(sensor_info, 4)
#endif

#if defined(CONFIG_SENSOR_ASYNC_API)
	ITERABLE_SECTION_ROM(sensor_decoder_api, 4)
#endif

#if defined(CONFIG_MCUMGR)
	ITERABLE_SECTION_ROM(mcumgr_handler, 4)
#endif

#if defined(CONFIG_INPUT)
	ITERABLE_SECTION_ROM(input_callback, 4)
#endif

#if defined(CONFIG_EMUL)
	ITERABLE_SECTION_ROM(emul, 4)
#endif /* CONFIG_EMUL */

#if defined(CONFIG_ZBUS)
	ITERABLE_SECTION_ROM(zbus_channel, 4)
	ITERABLE_SECTION_ROM(zbus_observer, 4)
	ITERABLE_SECTION_ROM(zbus_channel_observation, 4)
#endif /* CONFIG_ZBUS */

#ifdef CONFIG_LLEXT
	ITERABLE_SECTION_ROM(llext_const_symbol, 4)
#endif /* CONFIG_LLEXT */

	SECTION_DATA_PROLOGUE(symbol_to_keep,,)
	{
		__symbol_to_keep_start = .;
		KEEP(*(SORT(.symbol_to_keep*)));
		__symbol_to_keep_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	ITERABLE_SECTION_ROM(shell, 4)

	ITERABLE_SECTION_ROM(shell_root_cmds, 4)

	ITERABLE_SECTION_ROM(shell_subcmds, 4)

	ITERABLE_SECTION_ROM(shell_dynamic_subcmds, 4)

	ITERABLE_SECTION_ROM(cfb_font, 4)

#if defined(CONFIG_GNSS)
	ITERABLE_SECTION_ROM(gnss_data_callback, 4)
#endif

#if defined(CONFIG_GNSS_SATELLITES)
	ITERABLE_SECTION_ROM(gnss_satellites_callback, 4)
#endif
