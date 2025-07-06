# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)

if(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP OR
   CONFIG_BOARD_NRF54H20DK_NRF54H20_CPURAD OR
   CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP_IRON OR
   CONFIG_BOARD_NRF54H20DK_NRF54H20_CPURAD_IRON)
  if(CONFIG_SOC_NRF54H20_CPUAPP)
    set(JLINKSCRIPTFILE ${CMAKE_CURRENT_LIST_DIR}/support/nrf54h20_cpuapp.JLinkScript)
  else()
    set(JLINKSCRIPTFILE ${CMAKE_CURRENT_LIST_DIR}/support/nrf54h20_cpurad.JLinkScript)
  endif()

  board_runner_args(jlink "--device=CORTEX-M33" "--speed=4000" "--tool-opt=-jlinkscriptfile ${JLINKSCRIPTFILE}")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()

if(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUPPR OR CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUFLPR)
  if(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUPPR)
    set(JLINKSCRIPTFILE ${CMAKE_CURRENT_LIST_DIR}/support/nrf54h20_cpuppr.JLinkScript)
  else()
    set(JLINKSCRIPTFILE ${CMAKE_CURRENT_LIST_DIR}/support/nrf54h20_cpuflpr.JLinkScript)
  endif()

  board_runner_args(jlink "--device=RISC-V" "--speed=4000" "-if SW" "--tool-opt=-jlinkscriptfile ${JLINKSCRIPTFILE}")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()
