# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_KERNEL_SHELL
  kernel_service
)

zephyr_sources_ifdef(
  CONFIG_DEVICE_SHELL
  device_service.c
  )
zephyr_sources_ifdef(
  CONFIG_DATE_SHELL
  date_service.c
  )
zephyr_sources_ifdef(
  CONFIG_DEVMEM_SHELL
  devmem_service.c
  )
zephyr_sources_ifdef(
  CONFIG_APP_VERSION_SHELL
  app_version_service.c
  )
