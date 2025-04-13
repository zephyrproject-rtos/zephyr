# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BCM2711_VC_IF
    ../common/board_rev.c
)
zephyr_include_directories_ifdef(CONFIG_BCM2711_VC_IF
    ../common
)
