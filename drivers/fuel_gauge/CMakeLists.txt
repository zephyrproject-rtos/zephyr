# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_SBS_GAUGE_NEW_API		sbs_gauge)
add_subdirectory_ifdef(CONFIG_MAX17048		max17048)

zephyr_library_sources_ifdef(CONFIG_USERSPACE fuel_gauge_syscall_handlers.c)
