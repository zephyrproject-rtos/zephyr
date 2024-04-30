# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP OR CONFIG_BOARD_NRF54H20DK_NRF54H20_CPURAD)
  if(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPURAD)
    set(
      JLINK_TOOL_OPT
      "-jlinkscriptfile ${CMAKE_CURRENT_LIST_DIR}/support/nrf54h20_cpurad.JLinkScript"
    )
  endif()

  board_runner_args(jlink "--device=CORTEX-M33" "--speed=4000" "--tool-opt=${JLINK_TOOL_OPT}")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()
