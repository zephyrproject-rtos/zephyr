# SPDX-License-Identifier: Apache-2.0
if(CONFIG_BUILD_WITH_TFM)
  # Flash merged TF-M + Zephyr binary
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)

  # Flash is set all secure (<build>/tfm/api_ns/regression.sh) before flash programming
  # hence locate non-secure image from the secure flash base address in HEX file.
  dt_chosen(chosen_part_path PROPERTY "zephyr,code-partition")
  dt_reg_addr(chosen_part_addr PATH "${chosen_part_path}")
  math(EXPR TFM_HEX_BASE_ADDRESS_NS
    "${chosen_part_addr} - ${CONFIG_FLASH_BASE_ADDRESS} + ${CONFIG_STM32_INT_FLASH_SECURE_BASE_ADDRESS}"
  )
endif()

# keep first
if(CONFIG_STM32_MEMMAP)
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(stm32cubeprogrammer "--extload=MX25LM51245G_STM32U585I-IOT02A.stldr")
else()
board_runner_args(stm32cubeprogrammer "--erase" "--port=swd" "--reset-mode=hw")
endif()

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")

board_runner_args(jlink "--device=STM32U585AI" "--reset-after-load")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
# FIXME: openocd runner requires use of STMicro openocd fork.
# Check board documentation for more details.
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
