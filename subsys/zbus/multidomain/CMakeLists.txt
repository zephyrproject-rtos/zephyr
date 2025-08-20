# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(zbus_multidomain.c)

if(CONFIG_ZBUS_MULTIDOMAIN_UART)
  zephyr_library_sources(zbus_multidomain_uart.c)
endif()

if(CONFIG_ZBUS_MULTIDOMAIN_IPC)
  zephyr_library_sources(zbus_multidomain_ipc.c)
endif()
