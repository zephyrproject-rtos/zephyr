# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_TASK_WDT task_wdt.c)
zephyr_sources_ifdef(CONFIG_TASK_WDT_SHELL task_wdt_shell.c)
