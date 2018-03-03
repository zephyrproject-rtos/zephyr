
zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_RTT_CONSOLE rtt/SEGGER_RTT.c)
zephyr_sources_ifdef(CONFIG_SEGGER_SYSTEMVIEW systemview/SEGGER_SYSVIEW.c)
