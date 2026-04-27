# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFM)
  # Flash merged TF-M + Zephyr binary
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)

  # Flash is set all secure (<build>/tfm/api_ns/regression.sh) before flash programming
  # hence locate non-secure image from the secure flash base address in HEX file.
  set(flash_secure_base_address 0x0c000000)
  math(EXPR TFM_HEX_BASE_ADDRESS_NS "${flash_secure_base_address}+${CONFIG_FLASH_LOAD_OFFSET}")
endif()

# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(pyocd "--target=stm32l552zetxq")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
