# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

#[=======================================================================[.rst:
FindBabbleSim
*************

Find the :ref:`BabbleSim <bsim>` simulator and verify its version.

This module locates the BabbleSim simulator components and output paths, and verifies
that the installed version meets the minimum requirements.

Variables
=========

When the module is found, the following variables are set:

* :cmake:variable:`BSIM_COMPONENTS_PATH`
* :cmake:variable:`BSIM_OUT_PATH`

Search Process
==============

The module searches for BabbleSim in the following order:

1. Using the environment variables :envvar:`BSIM_COMPONENTS_PATH` and :envvar:`BSIM_OUT_PATH`
2. Using west to locate BabbleSim if it was fetched using the manifest

If BabbleSim cannot be found, the module will error with instructions on how to install it.

Version Checking
================

The module checks that the installed BabbleSim components meet the minimum version requirements.
Currently, it verifies that ``bs_2G4_phy_v1`` is at least version 2.4.

Example Usage
=============

.. code-block:: cmake

   find_package(BabbleSim)

   if(BabbleSim_FOUND)
     message(STATUS "BabbleSim found at ${BSIM_COMPONENTS_PATH}")
     message(STATUS "BabbleSim out path at ${BSIM_OUT_PATH}")
   endif()

#]=======================================================================]

zephyr_get(BSIM_COMPONENTS_PATH)
zephyr_get(BSIM_OUT_PATH)

if((DEFINED WEST) AND (NOT DEFINED BSIM_COMPONENTS_PATH) AND (NOT DEFINED BSIM_OUT_PATH))
  # Let's ask west for the bsim_project existence and its path
  execute_process(COMMAND ${WEST}
    status babblesim_base
    OUTPUT_QUIET
    ERROR_QUIET
    RESULT_VARIABLE ret_val1
  )
  execute_process(COMMAND ${WEST}
    list babblesim_base -f {posixpath}
    OUTPUT_VARIABLE BSIM_BASE_PATH
    ERROR_QUIET
    RESULT_VARIABLE ret_val2
  )
  if(NOT (${ret_val1} OR ${ret_val2}))
    string(STRIP ${BSIM_BASE_PATH} BSIM_COMPONENTS_PATH)
    get_filename_component(BSIM_OUT_PATH ${BSIM_COMPONENTS_PATH}/.. ABSOLUTE)
  endif()
endif()

message(STATUS "Using BSIM from BSIM_COMPONENTS_PATH=${BSIM_COMPONENTS_PATH} "
  "BSIM_OUT_PATH=${BSIM_OUT_PATH}")

if((NOT DEFINED BSIM_COMPONENTS_PATH) OR (NOT DEFINED BSIM_OUT_PATH))
  message(FATAL_ERROR
    "This board requires the BabbleSim simulator. Please either\n"
    " a) Enable the west babblesim group with\n"
    "    west config manifest.group-filter +babblesim && west update\n"
    "    and build it with\n"
    "    cd ${ZEPHYR_BASE}/../tools/bsim\n"
    "     make everything -j 8\n"
    "    OR\n"
    " b) set the environment variable BSIM_COMPONENTS_PATH to point to your own bsim installation\n"
    "    `components/` folder, *and* BSIM_OUT_PATH to point to the folder where the simulator\n"
    "    is compiled to.\n"
    "    More information can be found in https://babblesim.github.io/folder_structure_and_env.html"
  )
endif()

# Many apps cmake files (in and out of tree) expect these environment variables. Lets provide them:
set(ENV{BSIM_COMPONENTS_PATH} ${BSIM_COMPONENTS_PATH})
set(ENV{BSIM_OUT_PATH} ${BSIM_OUT_PATH})

# Let's check that it is new enough and built,
# so we provide better information to users than a compile error:

# Internal function to print a descriptive error message
# Do NOT use it outside of this module. It uses variables internal to it
function(bsim_handle_not_built_error)
  get_filename_component(BSIM_ROOT_PATH ${BSIM_COMPONENTS_PATH}/.. ABSOLUTE)
  message(FATAL_ERROR
    "Please ensure you have the latest babblesim and rebuild it.\n"
    "If you got it from Zephyr's manifest, you can do:\n"
    " west update\n"
    " cd ${BSIM_ROOT_PATH}; make everything -j 8"
  )
endfunction(bsim_handle_not_built_error)

# Internal function to check that a bsim component is at least the desired version
# Do NOT use it outside of this module. It uses variables internal to it
function(bsim_check_build_version bs_comp req_comp_ver)
  set(version_file ${BSIM_OUT_PATH}/lib/${bs_comp}.version)
  if(EXISTS ${version_file})
    file(READ ${version_file} found_version)
    string(STRIP ${found_version} found_version)
  else()
    message(WARNING "BabbleSim was never compiled (${version_file} not found)")
    bsim_handle_not_built_error()
  endif()

  if(found_version VERSION_LESS req_comp_ver)
    message(WARNING "Built ${bs_comp} version = ${found_version} < ${req_comp_ver} "
      "(required version)")
    bsim_handle_not_built_error()
  endif()
endfunction(bsim_check_build_version)

bsim_check_build_version(bs_2G4_phy_v1 2.4)
