# u-blox EVK-NORA-B10 board configuration

# Copyright (c) 2021 u-blox AG
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_UBX_EVKNORAB10_NRF5340_CPUAPP_NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_BOARD_UBX_EVKNORAB10_NRF5340_CPUAPP OR CONFIG_BOARD_UBX_EVKNORAB10_NRF5340_CPUAPP_NS)
board_runner_args(jlink "--device=nrf5340_xxaa_app" "--speed=4000")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file "${CMAKE_BINARY_DIR}/tfm_merged.hex")
endif()

if(CONFIG_BOARD_UBX_EVKNORAB10_NRF5340_CPUNET)
board_runner_args(jlink "--device=nrf5340_xxaa_net" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
