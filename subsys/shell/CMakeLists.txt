zephyr_include_directories_ifdef(CONFIG_CONSOLE_SHELL
  ${ZEPHYR_BASE}/include/drivers
  )

zephyr_sources(
  shell_service.c
  legacy_shell.c
  )

add_subdirectory(modules)
