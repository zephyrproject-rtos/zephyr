
target_sources_ifdef(CONFIG_OPENOCD_SUPPORT 		zephyr PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/openocd.c)
