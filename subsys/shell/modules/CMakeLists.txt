# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_KERNEL_SHELL
  kernel_service.c
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
