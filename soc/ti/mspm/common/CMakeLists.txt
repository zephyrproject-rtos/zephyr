# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_include_directories(.)
zephyr_include_directories(../../mspm_am13)

zephyr_sources_ifdef(CONFIG_REBOOT reboot.c)
zephyr_sources_ifdef(CONFIG_PM power.c)
zephyr_sources_ifdef(CONFIG_POWEROFF poweroff.c)
zephyr_sources(../../mspm_am13/soc_peripheral_delay.c)
