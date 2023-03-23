# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Let's try to find the simulator

zephyr_get(BSIM_COMPONENTS_PATH)
zephyr_get(BSIM_OUT_PATH)

if ((DEFINED WEST) AND (NOT DEFINED BSIM_COMPONENTS_PATH) AND (NOT DEFINED BSIM_OUT_PATH))
  # Let's ask west for the bsim_project existence and its path
  execute_process(COMMAND ${WEST}
    status babblesim_base
    OUTPUT_QUIET
    RESULT_VARIABLE ret_val1)
  execute_process(COMMAND ${WEST}
    list babblesim_base -f {posixpath}
    OUTPUT_VARIABLE BSIM_BASE_PATH
    RESULT_VARIABLE ret_val2)
  if (NOT (${ret_val1} OR ${ret_val2}))
    string(STRIP ${BSIM_BASE_PATH} BSIM_COMPONENTS_PATH)
    get_filename_component(BSIM_OUT_PATH ${BSIM_COMPONENTS_PATH}/.. ABSOLUTE)
  endif()
endif()

message(STATUS "Using BSIM from BSIM_COMPONENTS_PATH=${BSIM_COMPONENTS_PATH}\
 BSIM_OUT_PATH=${BSIM_OUT_PATH}")

if ((NOT DEFINED BSIM_COMPONENTS_PATH) OR (NOT DEFINED BSIM_OUT_PATH))
  message(FATAL_ERROR "This board requires the BabbleSim simulator. Please either\n\
  a) Enable the west babblesim group with\n\
     west config manifest.group-filter +babblesim && west update\n\
     and build it with\n\
     cd ${ZEPHYR_BASE}/../tools/bsim\n\
      make everything \n\
     OR\n\
  b) set the environment variable BSIM_COMPONENTS_PATH to point to your own bsim installation\n\
     `components/` folder, *and* BSIM_OUT_PATH to point to the folder where the simulator\n\
     is compiled to.\n\
     More information can be found in https://babblesim.github.io/folder_structure_and_env.html"
  )
endif()

set(ENV{BSIM_COMPONENTS_PATH} ${BSIM_COMPONENTS_PATH})
set(ENV{BSIM_OUT_PATH} ${BSIM_OUT_PATH})

