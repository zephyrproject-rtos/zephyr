# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)

if(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP OR CONFIG_BOARD_NRF54H20DK_NRF54H20_CPURAD)
  if(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP)
    set(JLINKSCRIPTFILE ${CMAKE_CURRENT_LIST_DIR}/support/nrf54h20_cpuapp.JLinkScript)
  else()
    set(JLINKSCRIPTFILE ${CMAKE_CURRENT_LIST_DIR}/support/nrf54h20_cpurad.JLinkScript)
  endif()

  board_runner_args(jlink "--device=CORTEX-M33" "--speed=4000" "--tool-opt=-jlinkscriptfile ${JLINKSCRIPTFILE}")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()
