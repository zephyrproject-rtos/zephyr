#
# Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

# PD mode specific sources
zephyr_library_sources_ifdef(CONFIG_OSDP_MODE_PD src/osdp_pd.c)

# CP mode specific sources
zephyr_library_sources_ifdef(CONFIG_OSDP_MODE_CP src/osdp_cp.c)

# Secure Channel sources
zephyr_library_sources_ifdef(CONFIG_OSDP_SC_ENABLED src/osdp_sc.c)

# Common sources
zephyr_library_sources_ifdef(CONFIG_OSDP
	src/osdp_phy.c
	src/osdp_common.c
	src/osdp.c
)
