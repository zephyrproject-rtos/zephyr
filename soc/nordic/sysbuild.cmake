# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Wire a launcher image to a VPR application built with CONFIG_GENERATE_INC_FILE_FOR_TARGET.
#
# Usage:
#   nordic_vpr_launcher_from_array(<launcher_image> <vpr_app>)
#
# Ensures <vpr_app> is configured and built before <launcher_image>, passes the remote
# generated include directory to the launcher, and enables CONFIG_NORDIC_VPR_LAUNCHER_FROM_ARRAY.
function(nordic_vpr_launcher_from_array launcher_image vpr_app)
  if(SB_CONFIG_VPR_LAUNCHER_FROM_ARRAY)
    sysbuild_add_dependencies(CONFIGURE ${launcher_image} ${vpr_app})
    add_dependencies(${launcher_image} ${vpr_app})

    set(remote_inc_dir "${CMAKE_BINARY_DIR}/${vpr_app}/zephyr/include/generated")
    set_config_string(${launcher_image} CONFIG_NRF_REMOTE_IMAGE_INC_DIR "${remote_inc_dir}")
    set_config_bool(${launcher_image} CONFIG_NORDIC_VPR_LAUNCHER_FROM_ARRAY 1)
    set_config_bool(${vpr_app} CONFIG_GENERATE_INC_FILE_FOR_TARGET 1)
  endif()
endfunction()

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

  if(NOT coproc_snippet MATCHES ".*-xip$")
    nordic_vpr_launcher_from_array(coproc_launcher ${DEFAULT_IMAGE})
  endif()
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
