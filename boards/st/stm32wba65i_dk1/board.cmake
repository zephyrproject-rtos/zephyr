# Copyright (c) 2025 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFM)
  set(FLASH_BASE_ADDRESS_S 0x0C000000)

  # Flash merged TF-M + Zephyr binary
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)

  if (CONFIG_HAS_FLASH_LOAD_OFFSET)
    MATH(EXPR TFM_HEX_BASE_ADDRESS_NS "${FLASH_BASE_ADDRESS_S}+${CONFIG_FLASH_LOAD_OFFSET}")
  else()
    set(TFM_HEX_BASE_ADDRESS_NS ${TFM_FLASH_BASE_ADDRESS_S})
  endif()

  # System entry point is TF-M vector, located 1kByte after tfm_fmw_partition in DTS
  get_target_property(TFM_FWM_NODE_NAME devicetree_target "DT_NODELABEL|slot0_secure_partition")
  string(REGEX REPLACE ".*@([^@]+)$" "\\1" TFM_FWM_OFFSET "${TFM_FWM_NODE_NAME}")
  if(NOT TFM_FWM_OFFSET)
    message(FATAL_ERROR "Could not find TF-M firmware offset from node label slot0_secure_partition")
  endif()
  math(EXPR TFM_FWM_BOOT_ADDR "0x${TFM_FWM_OFFSET}+${FLASH_BASE_ADDRESS_S}+0x400")

  board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw"
    "--erase" "--start-address=${TFM_FWM_BOOT_ADDR}"
  )
else()
  board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
endif()

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
