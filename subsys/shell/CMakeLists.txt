# SPDX-License-Identifier: Apache-2.0

add_subdirectory(modules)
add_subdirectory(backends)

if(CONFIG_SHELL_REMOTE_CLI)
  zephyr_sources_ifdef(
    CONFIG_SHELL
    shell_remote_cli.c
    shell_utils.c)
else()
  zephyr_sources_ifdef(
    CONFIG_SHELL
    shell.c
    shell_fprintf.c
    shell_utils.c
    shell_ops.c
    )
endif()

zephyr_sources_ifdef(CONFIG_SHELL_REMOTE shell_remote.c)

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
