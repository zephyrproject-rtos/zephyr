# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_sources_ifdef(CONFIG_CAN_MCUX_MCAN    can_mcux_mcan.c)

zephyr_library_sources_ifdef(CONFIG_CAN              can_common.c)
zephyr_library_sources_ifdef(CONFIG_CAN_LOOPBACK     can_loopback.c)
zephyr_library_sources_ifdef(CONFIG_CAN_MCAN         can_mcan.c)
zephyr_library_sources_ifdef(CONFIG_CAN_MCP2515      can_mcp2515.c)
zephyr_library_sources_ifdef(CONFIG_CAN_MCUX_FLEXCAN can_mcux_flexcan.c)
zephyr_library_sources_ifdef(CONFIG_CAN_SAM          can_sam.c)
zephyr_library_sources_ifdef(CONFIG_CAN_STM32        can_stm32.c)
zephyr_library_sources_ifdef(CONFIG_CAN_STM32FD      can_stm32fd.c)
zephyr_library_sources_ifdef(CONFIG_CAN_STM32H7      can_stm32h7.c)
zephyr_library_sources_ifdef(CONFIG_CAN_RCAR         can_rcar.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE        can_handlers.c)
zephyr_library_sources_ifdef(CONFIG_CAN_SHELL        can_shell.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_CAN  can_socketcan.c)

add_subdirectory(transceiver)
