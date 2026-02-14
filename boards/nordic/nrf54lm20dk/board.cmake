# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NRF54LM20A_CPUAPP OR CONFIG_SOC_NRF54LM20B_CPUAPP)
  board_runner_args(jlink "--device=nRF54LM20A_M33" "--speed=4000")
elseif(CONFIG_SOC_NRF54LM20A_CPUFLPR OR CONFIG_SOC_NRF54LM20B_CPUFLPR)
  board_runner_args(jlink "--device=nRF54LM20A_RV32" "--speed=4000")
endif()

if(CONFIG_TRUSTED_EXECUTION_NONSECURE)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
