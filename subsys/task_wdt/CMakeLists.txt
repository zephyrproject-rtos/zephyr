# SPDX-License-Identifier: Apache-2.0

if (CONFIG_TASK_WDT_TEST)
    zephyr_sources(task_wdt_test.c)
    message(WARNING "
        Warning: the test Task Watchdog is enabled and provides only a dummy
        implementation.")
else()
    zephyr_sources_ifdef(CONFIG_TASK_WDT task_wdt.c)
endif()

zephyr_sources_ifdef(CONFIG_TASK_WDT_SHELL task_wdt_shell.c)
