# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_CONSOLE_SUBSYS tty.c)
zephyr_sources_ifdef(CONFIG_CONSOLE_GETCHAR getchar.c)
zephyr_sources_ifdef(CONFIG_CONSOLE_GETLINE getline.c)
