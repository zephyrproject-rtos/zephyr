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
  # This FCB is specific to the flash on this board, it won't work
  # for boards with different flash chips. If you flash this FCB
  # onto a board with a different flash chip you may break it.
  # See MCUXpresso config tools for making a correct one.
  zephyr_library_sources(W25Q512JVFIQ_FCB.c)
endif()

if(CONFIG_DT_HAS_NXP_ENET_MAC_ENABLED AND ${xtal32_status})
  message(FATAL_ERROR "Ethernet and external 32K clock source are "
    "mutually exclusive on FRDM_RW612 due to shared PCB nets "
    "between the ethernet PHY and the external oscillator")
endif()
