#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# mgmt_mcumgr_transport covers interface API, allowing to implement transports.
# It is exposed with mgmt_mcumgr interface.
add_library(mgmt_mcumgr_transport STATIC src/smp.c)

target_sources_ifdef(CONFIG_MCUMGR_SMP_REASSEMBLY mgmt_mcumgr_transport PRIVATE
  src/smp_reassembly.c
)
target_sources_ifdef(CONFIG_MCUMGR_SMP_BT mgmt_mcumgr_transport PRIVATE
  src/smp_bt.c
)
target_sources_ifdef(CONFIG_MCUMGR_SMP_SHELL mgmt_mcumgr_transport PRIVATE
  src/smp_shell.c
)
target_sources_ifdef(CONFIG_MCUMGR_SMP_UART mgmt_mcumgr_transport PRIVATE
  src/smp_uart.c
)
target_sources_ifdef(CONFIG_MCUMGR_SMP_UDP mgmt_mcumgr_transport PRIVATE
  src/smp_udp.c
)
target_sources_ifdef(CONFIG_MCUMGR_SMP_DUMMY mgmt_mcumgr_transport PRIVATE
  src/smp_dummy.c
)

if (CONFIG_MCUMGR_SMP_SHELL OR CONFIG_MCUMGR_SMP_UART)
  target_sources(mgmt_mcumgr_transport PRIVATE src/serial_util.c)
endif()

target_include_directories(mgmt_mcumgr_transport PUBLIC include)

target_link_libraries(mgmt_mcumgr_transport PRIVATE
  mgmt_mcumgr_mgmt mgmt_mcumgr_util mgmt_mcumgr_protocol
)

target_link_libraries(mgmt_mcumgr_transport PRIVATE zephyr_interface kernel)

target_link_libraries(mgmt_mcumgr INTERFACE mgmt_mcumgr_transport)
