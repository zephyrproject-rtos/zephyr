# Copyright (c) 2026 Norik Systems
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_TRUSTED_EXECUTION_NONSECURE)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

board_runner_args(pyocd "--target=nrf91")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
