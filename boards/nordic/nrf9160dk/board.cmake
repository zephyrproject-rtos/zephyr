# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NRF9160DK_NRF9160 OR CONFIG_BOARD_NRF9160DK_NRF9160_NS)
  if(CONFIG_BOARD_NRF9160DK_NRF9160_NS)
    set(TFM_PUBLIC_KEY_FORMAT "full")
  endif()

  if(CONFIG_TFM_FLASH_MERGED_BINARY)
    set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
  endif()

  board_runner_args(jlink "--device=nRF9160_xxAA" "--speed=4000")
  include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
elseif(CONFIG_BOARD_NRF9160DK_NRF52840)
  board_runner_args(jlink "--device=nRF52840_xxAA" "--speed=4000")
  include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/openocd-nrf5.board.cmake)
endif()
