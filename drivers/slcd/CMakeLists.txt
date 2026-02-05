# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/slcd_controller.h)

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_SLCD_GPIO gpio_slcd.c)
zephyr_library_sources_ifdef(CONFIG_SLCD_NXP_MCUX nxp_slcd.c)
# zephyr-keep-sorted-stop
