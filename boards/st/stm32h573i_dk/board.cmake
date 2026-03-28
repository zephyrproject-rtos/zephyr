# SPDX-License-Identifier: Apache-2.0
if(CONFIG_BUILD_WITH_TFM)
  set(FLASH_BASE_ADDRESS_S 0x0C000000)

  # Flash merged TF-M + Zephyr binary
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)

  if(CONFIG_HAS_FLASH_LOAD_OFFSET)
    MATH(EXPR TFM_HEX_BASE_ADDRESS_NS "${FLASH_BASE_ADDRESS_S}+${CONFIG_FLASH_LOAD_OFFSET}")
  else()
    set(TFM_HEX_BASE_ADDRESS_NS ${TFM_FLASH_BASE_ADDRESS_S})
  endif()
endif()

# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
if(CONFIG_STM32_MEMMAP OR (CONFIG_XIP AND CONFIG_BOOTLOADER_MCUBOOT))
  board_runner_args(stm32cubeprogrammer "--extload=MX25LM51245G_STM32H573I-DK-RevB-SFIx.stldr")
endif()

board_runner_args(pyocd "--target=stm32h573iikx")

board_runner_args(probe_rs "--chip=STM32H573II")

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")

board_runner_args(stlink_gdbserver "--apid=1")
board_runner_args(stlink_gdbserver "--extload=MX25LM51245G_STM32H573I-DK.stldr")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/probe-rs.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stlink_gdbserver.board.cmake)
# FIXME: official openocd runner not yet available.
