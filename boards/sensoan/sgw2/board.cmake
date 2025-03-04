# Copyright (c) 2019 Nordic Semiconductor ASA.
# Copyright (c) 2024 Sensoan Oy
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_SGW2_NRF9160 OR CONFIG_BOARD_SGW2_NRF9160_NS)
  if(CONFIG_BOARD_SGW2_NRF9160_NS)
    set(TFM_PUBLIC_KEY_FORMAT "full")
  endif()

  if(CONFIG_TFM_FLASH_MERGED_BINARY)
    set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
  endif()
  board_runner_args(jlink "--device=nRF9160_xxAA" "--speed=4000")
  include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
elseif(CONFIG_BOARD_SGW2_NRF5340_CPUAPP OR CONFIG_BOARD_SGW2_NRF5340_CPUAPP_NS OR CONFIG_BOARD_SGW2_NRF5340_CPUNET)
  if(CONFIG_BOARD_SGW2_NRF5340_CPUAPP_NS)
    set(TFM_PUBLIC_KEY_FORMAT "full")
  endif()

  if(CONFIG_BOARD_SGW2_NRF5340_CPUAPP OR CONFIG_BOARD_SGW2_NRF5340_CPUAPP_NS)
    board_runner_args(jlink "--device=nrf5340_xxaa_app" "--speed=4000")
  endif()

  if(CONFIG_TFM_FLASH_MERGED_BINARY)
    set_property(TARGET runners_yaml_props_target PROPERTY hex_file "${CMAKE_BINARY_DIR}/tfm_merged.hex")
  endif()

  if(CONFIG_BOARD_SGW2_NRF5340_CPUNET)
    board_runner_args(jlink "--device=nrf5340_xxaa_net" "--speed=4000")
  endif()

  include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()