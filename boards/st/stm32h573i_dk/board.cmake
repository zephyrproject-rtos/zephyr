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
