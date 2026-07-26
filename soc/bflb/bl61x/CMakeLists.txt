# Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

if(CONFIG_BT_BFLB_BL61X OR CONFIG_WIFI_BFLB)
  zephyr_sources(bflb_rf.c)
endif()

if(CONFIG_BT_BFLB_BL61X)
  zephyr_sources(ble/btblecontroller_port_hw.c)
  zephyr_linker_sources(RWDATA bflb_rwdata.ld)
endif()

if(CONFIG_WIFI_BFLB)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/${SOC_SERIES}/wifi
  )
  zephyr_sources(
    wifi/bflb_wifi_platform.c
    wifi/bflb_wl80211_platform.c
  )

  zephyr_linker_sources(SECTIONS wifi/bflb_wifi_sections.ld)
endif()
