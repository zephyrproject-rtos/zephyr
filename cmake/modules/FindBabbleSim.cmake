# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# FindBabbleSim module for locating BabbleSim
#
# The module defines the following variables:
#
# 'BSIM_COMPONENTS_PATH'
# Path to the BabbleSim components source folder
#
# 'BSIM_OUT_PATH'
# Path to the BabbleSim build output root path (under which libraries and binaries) are kept
#
# We first try to find it via the environment variables BSIM_OUT_PATH and BSIM_COMPONENTS_PATH.
# If these are not set, as a fallback we attempt to find it through west, in case the user
# fetched babblesim using the manifest.
# Note that what we find through the environment variables is meant to have precedence.
#
# If BabbleSim cannot be found we error right away with a message trying to guide users

zephyr_get(BSIM_COMPONENTS_PATH)
zephyr_get(BSIM_OUT_PATH)

if ((DEFINED WEST) AND (NOT DEFINED BSIM_COMPONENTS_PATH) AND (NOT DEFINED BSIM_OUT_PATH))
  # Let's ask west for the bsim_project existence and its path
  execute_process(COMMAND ${WEST}
    status babblesim_base
    OUTPUT_QUIET
    ERROR_QUIET
    RESULT_VARIABLE ret_val1)
  execute_process(COMMAND ${WEST}
    list babblesim_base -f {posixpath}
    OUTPUT_VARIABLE BSIM_BASE_PATH
    ERROR_QUIET
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
      make everything -j 8\n\
     OR\n\
  b) set the environment variable BSIM_COMPONENTS_PATH to point to your own bsim installation\n\
     `components/` folder, *and* BSIM_OUT_PATH to point to the folder where the simulator\n\
     is compiled to.\n\
     More information can be found in https://babblesim.github.io/folder_structure_and_env.html"
  )
endif()

#Many apps cmake files (in and out of tree) expect these environment variables. Lets provide them:
set(ENV{BSIM_COMPONENTS_PATH} ${BSIM_COMPONENTS_PATH})
set(ENV{BSIM_OUT_PATH} ${BSIM_OUT_PATH})

# Let's check that it is new enough and built,
# so we provide better information to users than a compile error:

# Internal function to print a descriptive error message
# Do NOT use it outside of this module. It uses variables internal to it
function(bsim_handle_not_built_error)
  get_filename_component(BSIM_ROOT_PATH ${BSIM_COMPONENTS_PATH}/.. ABSOLUTE)
  message(FATAL_ERROR "Please ensure you have the latest babblesim and rebuild it."
          "If you got it from Zephyr's manifest, you can do:\n\
 west update\n\
 cd ${BSIM_ROOT_PATH}; make everything -j 8\n"
  )
endfunction(bsim_handle_not_built_error)

# Internal function to check that a bsim component is at least the desired version
# Do NOT use it outside of this module. It uses variables internal to it
function(bsim_check_build_version bs_comp req_comp_ver)
  set(version_file ${BSIM_OUT_PATH}/lib/${bs_comp}.version)
  if (EXISTS ${version_file})
    file(READ ${version_file} found_version)
    string(STRIP ${found_version} found_version)
  else()
    message(WARNING "BabbleSim was never compiled (${version_file} not found)")
    bsim_handle_not_built_error()
  endif()

  if (found_version VERSION_LESS req_comp_ver)
    message(WARNING
            "Built ${bs_comp} version = ${found_version} < ${req_comp_ver} (required version)")
    bsim_handle_not_built_error()
  endif()
endfunction(bsim_check_build_version)

bsim_check_build_version(bs_2G4_phy_v1 2.4)
