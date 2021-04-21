if(CONFIG_BUILD_WITH_TFM)
  set(TFM_FLASH_BASE_ADDRESS 0x0C000000)

  if (CONFIG_HAS_FLASH_LOAD_OFFSET)
    MATH(EXPR TFM_HEX_BASE_ADDRESS_NS "${TFM_FLASH_BASE_ADDRESS}+${CONFIG_FLASH_LOAD_OFFSET}")
  else()
    set(TFM_HEX_BASE_ADDRESS_NS ${TFM_TFM_FLASH_BASE_ADDRESS})
  endif()
endif()

set_ifndef(BOARD_DEBUG_RUNNER pyocd)
set_ifndef(BOARD_FLASH_RUNNER pyocd)

board_runner_args(pyocd "--target=stm32l562qeixq")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
