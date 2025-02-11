# Copyright (c) 2019, PHYTEC Messtechnik GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SWDP_BITBANG_DRIVER swdp_bitbang.c)
