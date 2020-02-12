# SPDX-License-Identifier: Apache-2.0

# This file provides Zephyr Config Package functionality.
#
# The purpose of this files is to allow users to decide if they want to:
# - Use ZEPHYR_BASE environment setting for explicitly set select a zephyr installation
# - Support automatic Zephyr installation lookup through the use of find_package(ZEPHYR)

# First check to see if user has provided a Zephyr base manually.
# Set Zephyr base to environment setting.
# It will be empty if not set in environment.
set(ZEPHYR_BASE $ENV{ZEPHYR_BASE})

# Find out the current Zephyr base.
get_filename_component(CURRENT_ZEPHYR_DIR ${CMAKE_CURRENT_LIST_DIR}/../../.. ABSOLUTE)

if (ZEPHYR_BASE)
  # Get rid of any double folder string before comparison, as example, user provides
  # ZEPHYR_BASE=//path/to//zephyr_base/
  # must also work.
  get_filename_component(ZEPHYR_BASE ${ZEPHYR_BASE} ABSOLUTE)
else()
  # Zephyr base is not set in environment but currently used Zephyr is located within the same tree
  # as the caller (sample/test/application) code of find_package(Zephyr).
  # Thus we set a Zephyr base for looking up boilerplate.cmake faster.
  set(ZEPHYR_BASE ${CURRENT_ZEPHYR_DIR})
endif()

message("Including boilerplate: ${ZEPHYR_BASE}/cmake/app/boilerplate.cmake")
include(${ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
