# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/comparator.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_COMP_STM32 comp_stm32.c)
