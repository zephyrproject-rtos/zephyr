# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/zsai.h)

zephyr_library()

zephyr_library_sources(zsai_ioctl.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE zsai_handlers.c)
