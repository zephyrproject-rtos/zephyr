# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_MODEM_RECEIVER modem_receiver.c)
zephyr_sources_ifdef(CONFIG_MODEM_SHELL modem_shell.c)

zephyr_sources_ifdef(CONFIG_MODEM_CONTEXT
	modem_context.c
	modem_pin.c
)

zephyr_sources_ifdef(CONFIG_MODEM_IFACE_UART modem_iface_uart.c)
zephyr_sources_ifdef(CONFIG_MODEM_CMD_HANDLER modem_cmd_handler.c)
zephyr_sources_ifdef(CONFIG_MODEM_SOCKET modem_socket.c)

if(CONFIG_MODEM_UBLOX_SARA)
	zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
	zephyr_library_sources(ublox-sara-r4.c)
endif()

if(CONFIG_MODEM_QUECTEL_BG9X)
	zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
	zephyr_library_sources(quectel-bg9x.c)
endif()

if(CONFIG_MODEM_WNCM14A2A)
	zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
	zephyr_library_sources(wncm14a2a.c)
endif()

if(CONFIG_MODEM_GSM_PPP)
	zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
	zephyr_library_sources(gsm_ppp.c)
endif()

if (CONFIG_MODEM_HL7800)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
  zephyr_library_sources(hl7800.c)
endif()
