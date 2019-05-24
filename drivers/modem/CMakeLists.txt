# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_MODEM_RECEIVER modem_receiver.c)
zephyr_sources_ifdef(CONFIG_MODEM_SHELL modem_shell.c)

if(CONFIG_MODEM_UBLOX_SARA_R4)
	zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
	zephyr_library_sources(ublox-sara-r4.c)
endif()

if(CONFIG_MODEM_WNCM14A2A)
	zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
	zephyr_library_sources(wncm14a2a.c)
endif()
