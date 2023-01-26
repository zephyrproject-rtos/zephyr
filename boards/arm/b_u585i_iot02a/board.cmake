# SPDX-License-Identifier: Apache-2.0
if(CONFIG_BUILD_WITH_TFM)
  set(TFM_FLASH_BASE_ADDRESS 0x0C000000)

  # Flash merged TF-M + Zephyr binary
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file "${CMAKE_BINARY_DIR}/tfm_merged.hex")

  if (CONFIG_HAS_FLASH_LOAD_OFFSET)
    MATH(EXPR TFM_HEX_BASE_ADDRESS_NS "${TFM_FLASH_BASE_ADDRESS}+${CONFIG_FLASH_LOAD_OFFSET}")
  else()
    set(TFM_HEX_BASE_ADDRESS_NS ${TFM_TFM_FLASH_BASE_ADDRESS})
  endif()
endif()

board_runner_args(stm32cubeprogrammer "--erase" "--port=swd" "--reset-mode=hw")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
# FIXME: openocd runner requires use of STMicro openocd fork.
# Check board documentation for more details.
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
