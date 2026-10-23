# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_SEVEN_NRF9151_NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

board_runner_args(probe-rs "--chip=nRF9151_xxAA")
board_set_debugger_ifnset(probe-rs)
include(${ZEPHYR_BASE}/boards/common/probe-rs.board.cmake)
