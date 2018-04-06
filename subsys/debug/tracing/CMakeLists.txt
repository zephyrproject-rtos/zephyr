zephyr_include_directories_ifdef(CONFIG_TRACING include)

zephyr_sources_ifdef(
  CONFIG_SEGGER_SYSTEMVIEW
  sysview_config.c
  sysview.c
  )
