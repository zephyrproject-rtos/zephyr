zephyr_sources_ifdef(CONFIG_KERNEL_SHELL
  kernel_service.c
  )
zephyr_sources_ifdef(CONFIG_LOGGER_SHELL
  logger_service.c
  )
