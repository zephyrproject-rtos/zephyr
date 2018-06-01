# Copyright (c) 2019, PHYTEC Messtechnik GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SWD_BITBANG_DRIVER swd_bitbang.c)
