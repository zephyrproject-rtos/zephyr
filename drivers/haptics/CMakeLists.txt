# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/haptics.h)

zephyr_library_sources_ifdef(CONFIG_HAPTICS_SHELL haptics_shell.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE haptics_handlers.c)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_HAPTICS_CS40L5X cs40l5x.c cs40l5x-i2c.c cs40l5x-spi.c)
zephyr_library_sources_ifdef(CONFIG_HAPTICS_DRV2605 drv2605.c)
# zephyr-keep-sorted-stop
