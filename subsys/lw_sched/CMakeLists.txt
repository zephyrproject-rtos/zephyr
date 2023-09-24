# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(${ZEPHYR_BASE}/subsys/lw_scheduler)

zephyr_library_sources(lw_scheduler.c)
zephyr_library_sources(lw_task.c)
