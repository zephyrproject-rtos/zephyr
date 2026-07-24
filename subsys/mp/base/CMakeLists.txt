#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MP_BASE)
    # zephyr-keep-sorted-start
    zephyr_library_sources_ifdef(CONFIG_MP_BASE_CAPSFILTER mp_capsfilter.c)
    zephyr_library_sources_ifdef(CONFIG_MP_BASE_QUEUE mp_queue.c)
    zephyr_library_sources_ifdef(CONFIG_MP_BASE_TEE mp_tee.c)
    # zephyr-keep-sorted-stop
endif()
