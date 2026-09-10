#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MPIPE_BASE)
  # zephyr-keep-sorted-start
  zephyr_library_sources_ifdef(CONFIG_MPIPE_BASE_CAPS_FILTER mpipe_caps_filter.c)
  zephyr_library_sources_ifdef(CONFIG_MPIPE_BASE_QUEUE mpipe_queue.c)
  zephyr_library_sources_ifdef(CONFIG_MPIPE_BASE_TEE mpipe_tee.c)
  # zephyr-keep-sorted-stop
endif()
