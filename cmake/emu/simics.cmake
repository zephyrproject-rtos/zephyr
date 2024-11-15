# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

find_program(
  SIMICS
  NAMES simics
  )

zephyr_get(SIMICS_SCRIPT_PATH SYSBUILD GLOBAL)
if(SIMICS_SCRIPT_PATH)
  set(SIMICS_SCRIPT ${SIMICS_SCRIPT_PATH})
else()
  set(SIMICS_SCRIPT ${BOARD_DIR}/support/${BOARD}.simics)
endif()

get_property(SIMICS_ARGS GLOBAL PROPERTY "BOARD_EMU_ARGS_simics")

add_custom_target(run_simics
  COMMAND
  ${SIMICS}
  -no-gui
  -no-win
  ${SIMICS_SCRIPT}
  ${SIMICS_ARGS}
  -e run
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  DEPENDS ${logical_target_for_zephyr_elf}
  USES_TERMINAL
  )
