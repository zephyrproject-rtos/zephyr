# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/nfc.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RC522 rc522.c)
