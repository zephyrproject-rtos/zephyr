# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_USE_RAIL soc_radio.c)
