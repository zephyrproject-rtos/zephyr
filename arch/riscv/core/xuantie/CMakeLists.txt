# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/cache.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_CACHE_XTHEADCMO cache_xtheadcmo.c)
zephyr_library_sources_ifdef(CONFIG_CACHE_XTHEADCMO_E907 cache_xtheadcmo_e907.c)
