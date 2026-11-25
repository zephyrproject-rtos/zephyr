# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NRF7120_CPUAPP OR CONFIG_SOC_NRF7120E_CPUAPP)
  board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
elseif(CONFIG_SOC_NRF7120_CPUFLPR OR CONFIG_SOC_NRF7120E_CPUFLPR)
  set(JLINKSCRIPTFILE ${CMAKE_CURRENT_LIST_DIR}/support/nrf7120_7120e_cpuflpr.JLinkScript)

  # Workaround: Use device nRF54L15_RV32 until dedicated nRF7120 RISC-V device support is added
  board_runner_args(jlink "--device=nRF54L15_RV32" "--speed=4000" "--tool-opt=-jlinkscriptfile ${JLINKSCRIPTFILE}")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
