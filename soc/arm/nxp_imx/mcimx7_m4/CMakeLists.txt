#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            soc.c
            soc_clk_freq.c
)

target_sources(soc_arm PRIVATE ${PRIVATE_SOURCES})
