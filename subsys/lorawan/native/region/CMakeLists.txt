# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(region.c)
zephyr_library_sources_ifdef(CONFIG_LORAWAN_REGION_EU868 region_eu868.c)
