# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_NORDIC_COPROC_LAUNCHER)
  # Add a variant of the coproc_launcher project tailored to launch the sysbuild board target.
  #
  # We use the sysbuild build target to determine the build target for the coproc_launcher project
  # along with the required snippet for the coproc_launcher project for launching the sysbuild
  # build target:
  #
  #   coproc_launcher_board: <board>/<soc>/cpuapp
  #   coproc_snippet:        nordic-<cpucluster>-<variant>

  string(REPLACE "/" ";" coproc_target_qualifiers ${BOARD_QUALIFIERS})
  list(GET coproc_target_qualifiers 0 coproc_soc)
  list(GET coproc_target_qualifiers 1 coproc_cpucluster)
  string(REPLACE "cpu" "" coproc_cpucluster ${coproc_cpucluster})
  list(POP_FRONT coproc_target_qualifiers)
  list(POP_FRONT coproc_target_qualifiers)
  list(PREPEND coproc_target_qualifiers ${coproc_cpucluster})
  string(JOIN "-" coproc_snippet "nordic" ${coproc_target_qualifiers})
  set(coproc_launcher_qualifiers "${coproc_soc}/cpuapp")
  set(coproc_launcher_board "${BOARD}/${coproc_launcher_qualifiers}")

  ExternalZephyrProject_Add(
    APPLICATION coproc_launcher
    SOURCE_DIR ${ZEPHYR_BASE}/samples/boards/nordic/coproc_launcher
    BOARD ${coproc_launcher_board}
    SNIPPET ${coproc_snippet}
  )
endif()

if(SB_CONFIG_NRF_GENERATE_UICR)
  include(${CMAKE_CURRENT_LIST_DIR}/common/uicr/sysbuild.cmake)
endif()

if(SB_CONFIG_SOC_NRF71_GENERATE_UICR)
  include(${CMAKE_CURRENT_LIST_DIR}/nrf71/uicr/sysbuild.cmake)
endif()

if(SB_CONFIG_SOC_NRF71_GENERATE_WICR)
  include(${CMAKE_CURRENT_LIST_DIR}/nrf71/wicr/sysbuild.cmake)
endif()
