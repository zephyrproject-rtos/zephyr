#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022-2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# mgmt_mcumgr_transport covers interface API, allowing to implement transports.
# It is exposed with mgmt_mcumgr interface.
zephyr_library_named(mcumgr_transport)
zephyr_library_sources(src/smp.c)

zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_REASSEMBLY
  src/smp_reassembly.c
)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT
  src/smp_bt.c
)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_SHELL
  src/smp_shell.c
)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_UART
  src/smp_uart.c
)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_RAW_UART
  src/smp_raw_uart.c
)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_UDP
  src/smp_udp.c
)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_LORAWAN
  src/smp_lorawan.c
)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_DUMMY
  src/smp_dummy.c
)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_RAW_DUMMY
  src/smp_raw_dummy.c
)

if(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_SMP_OVER_CONSOLE OR
  CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_RAW_BINARY_NON_SMP_OVER_CONSOLE
)
  zephyr_library_sources(src/serial_util.c)
endif()

zephyr_include_directories(include)
