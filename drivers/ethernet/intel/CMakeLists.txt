# Copyright (c) 2025 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ETH_INTEL_PLAT	eth_intel_plat.c)
zephyr_library_sources_ifdef(CONFIG_ETH_INTEL_IGC	eth_intel_igc.c)
