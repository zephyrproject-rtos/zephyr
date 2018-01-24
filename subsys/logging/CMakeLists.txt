zephyr_sources_ifdef(CONFIG_SYS_LOG sys_log.c)
zephyr_sources_ifdef(
  CONFIG_KERNEL_EVENT_LOGGER
  event_logger.c
  kernel_event_logger.c
  )
zephyr_sources_ifdef(CONFIG_SYS_LOG_BACKEND_NET sys_log_net.c)
