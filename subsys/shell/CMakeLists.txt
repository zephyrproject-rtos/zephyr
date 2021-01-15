# SPDX-License-Identifier: Apache-2.0

add_subdirectory(modules)

zephyr_sources_ifdef(
  CONFIG_SHELL
  shell.c
  shell_fprintf.c
  shell_utils.c
  shell_ops.c
  )

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_SERIAL
  shell_uart.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_DUMMY
  shell_dummy.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_RTT
  shell_rtt.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_TELNET
  shell_telnet.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_HELP
  shell_help.c
  )

zephyr_sources_ifdef(
  CONFIG_SHELL_GETOPT
  shell_getopt.c
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
  CONFIG_SHELL_LOG_BACKEND
  shell_log_backend.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_WILDCARD
  shell_wildcard.c
)
