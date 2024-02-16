# Copyright 2022-2023 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(osif.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_S32K3 power_soc.c)
