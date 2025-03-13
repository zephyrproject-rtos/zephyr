# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_HDLC_RCP_IF_NXP hdlc_rcp_if_nxp.c)
zephyr_library_sources_ifdef(CONFIG_HDLC_RCP_IF_UART hdlc_rcp_if_uart.c)
