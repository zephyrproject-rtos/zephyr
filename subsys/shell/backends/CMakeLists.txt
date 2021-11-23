# SPDX-License-Identifier: Apache-2.0

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
