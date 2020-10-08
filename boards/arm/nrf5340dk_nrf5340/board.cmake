# SPDX-License-Identifier: Apache-2.0

# Set the corresponding TF-M target platform when building for the Non-Secure
# version of the board (Application MCU).
if(CONFIG_BOARD_NRF5340PDK_NRF5340_CPUAPPNS)
  set(TFM_TARGET_PLATFORM "nordic_nrf/nrf5340pdk_nrf5340_cpuapp")
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if((CONFIG_BOARD_NRF5340PDK_NRF5340_CPUAPP OR CONFIG_BOARD_NRF5340PDK_NRF5340_CPUAPPNS) OR
  (CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP OR CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPPNS))
board_runner_args(nrfjprog "--nrf-family=NRF53" "--tool-opt=--coprocessor CP_APPLICATION")
board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
endif()

if(CONFIG_BOARD_NRF5340PDK_NRF5340_CPUNET OR CONFIG_BOARD_NRF5340DK_NRF5340_CPUNET)
board_runner_args(nrfjprog "--nrf-family=NRF53" "--tool-opt=--coprocessor CP_NETWORK")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
