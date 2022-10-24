# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)

zephyr_sources_ifdef(CONFIG_PM soc_power.c)
