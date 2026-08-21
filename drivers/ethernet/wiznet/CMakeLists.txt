# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ETH_W5100S eth_w5100s.c)
zephyr_library_sources_ifdef(CONFIG_ETH_W5500 eth_w5500.c)
zephyr_library_sources_ifdef(CONFIG_ETH_W6100 eth_w6100.c)
zephyr_library_sources_ifdef(CONFIG_ETH_W6300 eth_w6300.c)
zephyr_library_sources_ifdef(CONFIG_ETH_WIZNET eth_wiznet.c)
