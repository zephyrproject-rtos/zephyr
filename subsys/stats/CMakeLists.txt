# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_STATS stats.c)
zephyr_sources_ifdef(CONFIG_STATS_SHELL stats_shell.c)
