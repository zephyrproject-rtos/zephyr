# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_CONSOLE_GETCHAR getchar.c)
zephyr_sources_ifdef(CONFIG_CONSOLE_GETLINE getline.c)
zephyr_sources_ifdef(CONFIG_CONSOLE_RX_INTERRUPT_DRIVEN tty.c)
