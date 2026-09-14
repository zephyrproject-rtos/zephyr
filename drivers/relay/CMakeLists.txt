# Copyright (c) 2026 Siemens AG
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/relay/relay.h)

if(CONFIG_RELAY)
  zephyr_library()
  zephyr_library_sources_ifdef(CONFIG_USERSPACE relay_handlers.c)
  zephyr_library_sources_ifdef(CONFIG_RELAY_GPIO relay_gpio.c)
  zephyr_library_sources_ifdef(CONFIG_RELAY_SHELL relay_shell.c)
endif()
