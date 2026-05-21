# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_CPU_WORKLOAD_HISTORY history.c)
zephyr_library_sources_ifdef(CONFIG_CPU_WORKLOAD_THREAD_PROFILE thread.c)
