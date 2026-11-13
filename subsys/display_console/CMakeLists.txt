# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(display_console.c display_console_uart.c)
zephyr_sources_ifdef(CONFIG_DISPLAY_CONSOLE_KBD display_console_kbd.c)
zephyr_sources_ifdef(CONFIG_DISPLAY_CONSOLE_RENDERER_CFB display_console_cfb.c)
zephyr_sources_ifdef(CONFIG_DISPLAY_CONSOLE_RENDERER_LVGL display_console_lvgl.c)
