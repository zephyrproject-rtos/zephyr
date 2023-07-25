#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# mgmt_mcumgr_transport covers interface API, allowing to implement transports.
# It is exposed with mgmt_mcumgr interface.
zephyr_library(mgmt_mcumgr_transport)
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
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_UDP
  src/smp_udp.c
)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_DUMMY
  src/smp_dummy.c
)

if (CONFIG_MCUMGR_TRANSPORT_SHELL OR CONFIG_MCUMGR_TRANSPORT_UART)
  zephyr_library_sources(src/serial_util.c)
endif()

zephyr_include_directories(include)
