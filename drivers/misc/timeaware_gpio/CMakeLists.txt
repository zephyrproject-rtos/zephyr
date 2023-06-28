# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/misc/timeaware_gpio/timeaware_gpio.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE  timeaware_gpio_handlers.c)
