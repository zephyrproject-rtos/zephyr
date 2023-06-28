# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_MIMX93_A55 imx93_ccm.c)
zephyr_sources_ifdef(CONFIG_SOC_MIMX93_A55 imx93_ccm_clock_tree.c)
