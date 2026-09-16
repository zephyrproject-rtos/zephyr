#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_sources_ifdef(CONFIG_MPIPE_DUMP mpipe_dump.c)
zephyr_library_sources_ifdef(CONFIG_MPIPE_PLAYER mpipe_player.c)
