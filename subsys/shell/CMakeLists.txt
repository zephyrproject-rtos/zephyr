# SPDX-License-Identifier: Apache-2.0

add_subdirectory(modules)
add_subdirectory(backends)

zephyr_sources_ifdef(
  CONFIG_SHELL
  shell.c
  shell_fprintf.c
  shell_utils.c
  shell_ops.c
  )

if(NOT CONFIG_SHELL_LOG_BACKEND_CUSTOM)
  zephyr_sources_ifdef(
    CONFIG_SHELL_LOG_BACKEND
    shell_log_backend.c
  )
endif()

zephyr_sources_ifdef(
  CONFIG_SHELL_HELP
  shell_help.c
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
  CONFIG_SHELL_WILDCARD
  shell_wildcard.c
  )
