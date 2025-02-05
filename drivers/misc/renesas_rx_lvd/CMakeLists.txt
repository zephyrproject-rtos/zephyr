# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RENESAS_RX_LVD renesas_rx_lvd.c)
