zephyr_sources_ifdef(
  CONFIG_KERNEL_SHELL
  kernel_service.c
  )
zephyr_sources_ifdef(
  CONFIG_DEVICE_SHELL
  device_service.c
  )
