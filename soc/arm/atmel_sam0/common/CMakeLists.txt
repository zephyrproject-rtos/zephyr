# Makefile - Atmel SAM0 MCU family
#
# Copyright (c) 2019 ML!PA Consulting GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMD20 soc_samd2x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMD21 soc_samd2x.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_SAMR21 soc_samd2x.c)
