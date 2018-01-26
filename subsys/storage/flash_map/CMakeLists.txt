zephyr_sources(flash_map.c)
zephyr_sources_ifndef(CONFIG_FLASH_MAP_CUSTOM flash_map_default.c)

