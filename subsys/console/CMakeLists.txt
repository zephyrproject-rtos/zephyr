# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_CONSOLE_GETCHAR tty.c getchar.c)
zephyr_sources_ifdef(CONFIG_CONSOLE_GETLINE getline.c)
