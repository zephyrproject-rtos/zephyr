# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(zbus_proxy_agent.c)

if(CONFIG_ZBUS_PROXY_AGENT_IPC)
  zephyr_library_sources(zbus_proxy_agent_ipc.c)
endif()

if(CONFIG_ZBUS_PROXY_AGENT_UART)
  zephyr_library_sources(zbus_proxy_agent_uart.c)
endif()
