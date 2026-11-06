# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_PM power.c)
zephyr_sources_ifdef(CONFIG_POWEROFF poweroff.c)
