# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/comparator.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_COMPARATOR_NRFX_COMP comparator_nrfx_comp.c)
zephyr_library_sources_ifdef(CONFIG_COMPARATOR_NRFX_LPCOMP comparator_nrfx_lpcomp.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE comparator_handlers.c)
