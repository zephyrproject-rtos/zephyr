# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(common.c)
zephyr_library_sources_ifdef(CONFIG_PINCTRL_NXP_MCUX pinctrl_nxp_mcux.c)
