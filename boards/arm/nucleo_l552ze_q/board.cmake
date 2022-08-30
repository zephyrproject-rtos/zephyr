# SPDX-License-Identifier: Apache-2.0
if(CONFIG_BUILD_WITH_TFM)
  set(FLASH_BASE_ADDRESS_S 0x0C000000)

  # Flash merged TF-M + Zephyr binary
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file "${CMAKE_BINARY_DIR}/tfm_merged.hex")

  if (CONFIG_HAS_FLASH_LOAD_OFFSET)
    MATH(EXPR TFM_HEX_BASE_ADDRESS_NS "${FLASH_BASE_ADDRESS_S}+${CONFIG_FLASH_LOAD_OFFSET}")
  else()
    set(TFM_HEX_BASE_ADDRESS_NS ${TFM_FLASH_BASE_ADDRESS_S})
  endif()
endif()

set_ifndef(BOARD_DEBUG_RUNNER pyocd)
set_ifndef(BOARD_FLASH_RUNNER pyocd)

board_runner_args(pyocd "--target=stm32l552zetxq")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
