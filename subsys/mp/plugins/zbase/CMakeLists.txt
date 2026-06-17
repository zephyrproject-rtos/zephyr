#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_MP_CAPSFILTER mp_capsfilter.c)
zephyr_library_sources_ifdef(CONFIG_MP_QUEUE mp_queue.c)
zephyr_library_sources_ifdef(CONFIG_MP_TEE mp_tee.c)
# zephyr-keep-sorted-stop
