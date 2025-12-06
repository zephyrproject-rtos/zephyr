#
# Copyright 2022-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(init.c)
dt_nodelabel(xtal32 NODELABEL "xtal32")
dt_node_has_status(xtal32_status PATH ${xtal32} STATUS okay)

if(CONFIG_NXP_RW6XX_BOOT_HEADER)
  if(NOT DEFINED CONFIG_BOARD_RD_RW612_BGA)
    message(WARNING "It appears you are using the board definition for "
      "the RD_RW612_BGA, but targeting a custom board. You may need to "
      "update your flash configuration block data")
  endif()

  zephyr_library_sources(MX25U51245GZ4I00_FCB.c)
endif()

if(CONFIG_DT_HAS_NXP_ENET_MAC_ENABLED AND ${xtal32_status})
  message(FATAL_ERROR "Ethernet and external 32K clock source are "
    "mutually exclusive on RD_RW612_BGA due to shared PCB nets "
    "between the ethernet PHY and the external oscillator")
endif()
