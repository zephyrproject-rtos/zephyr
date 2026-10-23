# SPDX-License-Identifier: Apache-2.0

if(CONFIG_TASK_WDT_DUMMY)
  zephyr_sources(task_wdt_dummy.c)
  message(WARNING "Dummy Task Watchdog implementation is enabled.")
else()
  zephyr_sources_ifdef(CONFIG_TASK_WDT task_wdt.c)
  zephyr_sources_ifdef(CONFIG_TASK_WDT_RESET_INFO task_wdt_reset_info.c)
  zephyr_sources_ifdef(CONFIG_TASK_WDT_SUPERVISION task_wdt_supervision.c)
  zephyr_sources_ifdef(CONFIG_TASK_WDT_GROUPS task_wdt_group.c)
endif()

zephyr_sources_ifdef(CONFIG_TASK_WDT_SHELL task_wdt_shell.c)
