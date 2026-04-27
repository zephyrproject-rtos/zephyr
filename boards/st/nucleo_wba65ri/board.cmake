# Copyright (c) 2025 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFM)
  # Flash merged TF-M + Zephyr binary
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)

  # Flash is set all secure (<build>/tfm/api_ns/regression.sh) before flash programming
  # hence locate non-secure image from the secure flash base address in HEX file.
  set(flash_secure_base_address 0x0c000000)
  math(EXPR TFM_HEX_BASE_ADDRESS_NS "${flash_secure_base_address}+${CONFIG_FLASH_LOAD_OFFSET}")

  # System entry point is TF-M vector, located 1kByte after tfm_fmw_partition in DTS
  dt_nodelabel(tfm_partition_path NODELABEL slot0_secure_partition REQUIRED)
  dt_reg_addr(tfm_partition_offset PATH ${tfm_partition_path} REQUIRED)
  math(EXPR tfm_fwm_boot_address "${tfm_partition_offset}+${flash_secure_base_address}+0x400")

  board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw"
    "--erase" "--start-address=${tfm_fwm_boot_address}"
  )
else()
  board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
endif()

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
