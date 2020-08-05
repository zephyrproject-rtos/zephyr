# Copyright (c) 2019 Brett Witherspoon
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(ccfg.c)

zephyr_library_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT    power.c)
zephyr_library_sources_ifdef(CONFIG_DEVICE_POWER_MANAGEMENT   power.c)
