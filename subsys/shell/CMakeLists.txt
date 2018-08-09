zephyr_include_directories_ifdef(CONFIG_CONSOLE_SHELL
  ${ZEPHYR_BASE}/include/drivers
  )

zephyr_sources_ifdef(
  CONFIG_CONSOLE_SHELL
  shell_service.c
  legacy_shell.c
  )

add_subdirectory(modules)

zephyr_sources_ifdef(
  CONFIG_SHELL
  shell.c
  shell_fprintf.c
  shell_utils.c
  shell_ops.c
  shell_uart.c
  )

zephyr_sources_ifdef(
  CONFIG_SHELL_CMDS
  shell_cmds.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_HISTORY
  shell_history.c
)

zephyr_sources_ifdef(
  CONFIG_LOG
  shell_log_backend.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_WILDCARD
  shell_wildcard.c
)
