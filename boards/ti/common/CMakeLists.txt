# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_BOARD_CC1352_LP_SKY13317 board_antenna.c)
