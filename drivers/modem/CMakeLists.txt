# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MODEM_RECEIVER modem_receiver.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_SHELL modem_shell.c)

zephyr_library_sources_ifdef(CONFIG_MODEM_CONTEXT modem_context.c)

zephyr_library_sources_ifdef(CONFIG_MODEM_IFACE_UART_INTERRUPT modem_iface_uart_interrupt.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_IFACE_UART_ASYNC modem_iface_uart_async.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_CMD_HANDLER modem_cmd_handler.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_SOCKET modem_socket.c)

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/sockets)

if(CONFIG_MODEM_UBLOX_SARA)
	zephyr_library_sources(ublox-sara-r4.c)
endif()

if(CONFIG_MODEM_QUECTEL_BG9X)
	zephyr_library_sources(quectel-bg9x.c)
endif()

if(CONFIG_MODEM_WNCM14A2A)
	zephyr_library_sources(wncm14a2a.c)
endif()

if (CONFIG_MODEM_HL7800)
  zephyr_library_sources(hl7800.c)
endif()

if (CONFIG_MODEM_SIM7080)
	zephyr_library_sources(simcom-sim7080.c)
endif()

zephyr_library_sources_ifdef(CONFIG_MODEM_CELLULAR modem_cellular.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_AT_USER_PIPE modem_at_user_pipe.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_AT_SHELL modem_at_shell.c)
