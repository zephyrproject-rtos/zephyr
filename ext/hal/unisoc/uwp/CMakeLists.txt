if(CONFIG_HAS_UWP)
	add_subdirectory_ifdef(CONFIG_SIPC drivers/src/sipc)
	add_subdirectory_ifdef(CONFIG_BT_UWP5661 drivers/src/bt)
	add_subdirectory_ifdef(CONFIG_USE_UWP_HAL_SFC drivers/src/sfc)
	zephyr_include_directories(
		drivers/include
		soc
		)
	zephyr_sources(soc/system.c)
	zephyr_sources_ifdef(CONFIG_USE_UWP_HAL_CACHE soc/cache.c)
	zephyr_sources_ifdef(CONFIG_USE_UWP_HAL_GNSS soc/gnss.c)
	zephyr_sources_ifdef(CONFIG_USE_UWP_HAL_FW soc/load_fw.c)

	zephyr_sources_ifdef(CONFIG_PINMUX_UWP drivers/src/hal_pinmux.c)
endif()
