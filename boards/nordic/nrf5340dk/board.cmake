# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP_NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP OR CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP_NS)
board_runner_args(jlink "--device=nrf5340_xxaa_app" "--speed=4000")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

if(CONFIG_BOARD_NRF5340DK_NRF5340_CPUNET)
board_runner_args(jlink "--device=nrf5340_xxaa_net" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
