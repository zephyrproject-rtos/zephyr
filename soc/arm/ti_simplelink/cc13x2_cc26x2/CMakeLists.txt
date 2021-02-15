# Copyright (c) 2019 Brett Witherspoon
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(ccfg.c)

zephyr_library_sources_ifdef(CONFIG_PM    power.c)
zephyr_library_sources_ifdef(CONFIG_PM_DEVICE   power.c)
