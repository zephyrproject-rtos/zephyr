# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NRF54L15_CPUAPP)
  board_runner_args(pyocd "--target=nrf54l" "--frequency=4000000")
  board_runner_args(openocd "--cmd-load=nrf54l-load" "--cmd-erase=nrf54l_mass_erase" -c "targets nrf54l.cpu")
  board_runner_args(jlink "--device=nRF54L15_M33" "--speed=4000")
elseif(CONFIG_SOC_NRF54L15_CPUFLPR)
  board_runner_args(openocd "--cmd-load=nrf54l-load" "--cmd-erase=nrf54l_mass_erase" -c "targets nrf54l.aux")
  board_runner_args(jlink "--device=nRF54L15_RV32")
elseif(CONFIG_SOC_NRF52820_QDAA)
  board_runner_args(uf2 "--board-id=NRF54L15-DISCOVERY-IFMCU")
  board_runner_args(pyocd "--target=nrf52")
  include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
endif()

if(BOARD_NRF54L15_DISCOVERY_NRF54L15_CPUAPP_NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
