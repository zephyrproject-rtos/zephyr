# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/digipot.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DIGIPOT_AD528X		digipot_ad528x.c)
