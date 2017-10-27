zephyr_include_directories_ifdef(CONFIG_CONSOLE_SHELL
  $ENV{ZEPHYR_BASE}/include/drivers
  )

zephyr_sources(
  shell_service.c
  shell.c
  )

add_subdirectory(modules)
