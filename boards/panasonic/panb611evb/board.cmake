# Copyright (c) 2025 Panasonic Industrial Devices Europe GmbH
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NRF54L15_CPUAPP)
  board_runner_args(jlink "--device=nRF54L15_M33" "--speed=4000")
elseif(CONFIG_SOC_NRF54L15_CPUFLPR)
  board_runner_args(jlink "--device=nRF54L15_RV32")
endif()

if(CONFIG_BOARD_PANB611EVB_NRF54L15_CPUAPP_NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
