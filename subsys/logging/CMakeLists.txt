
target_sources_ifdef(CONFIG_SYS_LOG 		zephyr PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/sys_log.c)
target_sources_ifdef(CONFIG_KERNEL_EVENT_LOGGER zephyr PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/event_logger.c
	${CMAKE_CURRENT_LIST_DIR}/kernel_event_logger.c
	)
