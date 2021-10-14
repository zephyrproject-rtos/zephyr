# Copyright (c) 2019 Nordic Semiconductor ASA
# Copyright (c) 2021 Laird Connectivity
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_BL5340_DVK_CPUAPP_NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_BOARD_BL5340_DVK_CPUAPP OR CONFIG_BOARD_BL5340_DVK_CPUAPP_NS)
board_runner_args(jlink "--device=nrf5340_xxaa_app" "--speed=4000")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file "${CMAKE_BINARY_DIR}/tfm_merged.hex")
endif()

if(CONFIG_BOARD_BL5340_DVK_CPUNET)
board_runner_args(jlink "--device=nrf5340_xxaa_net" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
