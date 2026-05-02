# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_CPU_WORKLOAD_ESTIMATE estimate.c)
zephyr_library_sources_ifdef(CONFIG_CPU_WORKLOAD_READY_BACKLOG backlog.c)
zephyr_library_sources_ifdef(CONFIG_CPU_WORKLOAD_ARRIVAL_PROFILE arrival.c)
zephyr_library_sources_ifdef(CONFIG_CPU_WORKLOAD_WORK_PROFILE work.c)
