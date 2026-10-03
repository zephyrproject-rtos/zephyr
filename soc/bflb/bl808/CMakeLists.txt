# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

if(CONFIG_BT_BFLB_BL808 OR CONFIG_WIFI_BFLB)
  zephyr_sources(bflb_rf.c)
endif()

if(CONFIG_BT_BFLB_BL808)
  zephyr_sources(ble/btblecontroller_port_hw.c)
  zephyr_linker_sources(RWDATA bflb_rwdata.ld)
endif()

if(CONFIG_WIFI_BFLB)
  zephyr_include_directories(
    ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR}/include/bouffalolab/wifi4
    ${ZEPHYR_BASE}/drivers/wifi/bflb/wifi4
  )
  zephyr_sources(
    wifi/bflb_wifi_platform.c
    wifi/bflb_wifi_ipc.c
  )

  zephyr_linker_sources(SECTIONS wifi/bflb_wifi_sections.ld)
  zephyr_linker_sources(RAM_SECTIONS wifi/bflb_wifi_heap.ld)
endif()
