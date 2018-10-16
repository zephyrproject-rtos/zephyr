if(CONFIG_HAS_UWP)
	zephyr_include_directories(
		drivers/include
		)
	zephyr_sources(soc/system.c)
	zephyr_sources_ifdef(CONFIG_USE_UWP_HAL_CACHE soc/cache.c)
	zephyr_sources_ifdef(CONFIG_USE_UWP_HAL_GNSS soc/gnss.c)
	zephyr_sources_ifdef(CONFIG_USE_UWP_HAL_FW soc/load_fw.c)
endif()
