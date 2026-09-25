# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Core
zephyr_library_sources_ifdef(CONFIG_ETH_CDNS_MACB_CORE eth_cdns_macb_core.c)
