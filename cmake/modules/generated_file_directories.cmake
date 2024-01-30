# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

# This file creates locations in the build directory
# for placing generated files.
#
# Outcome:
# - BINARY_DIR is set to ${PROJECT_BINARY_DIR} or ${CMAKE_BINARY_DIR}
# - BINARY_DIR_INCLUDE is set to ${BINARY_DIR}/include
# - BINARY_DIR_INCLUDE_GENERATED is set to ${BINARY_DIR_INCLUDE}/generated
# - BINARY_DIR_INCLUDE_GENERATED_H is set to ${BINARY_DIR_INCLUDE_GENERATED}
# - BINARY_DIR_INCLUDE_GENERATED is a directory
#
# Required variables:
# None
#
# Optional variables:
# None
#
# Optional environment variables:
# None

set(INCLUDE_GENERATED_REL_DIR include/generated)
set(INCLUDE_GENERATED_REL_DIR_H ${INCLUDE_GENERATED_REL_DIR})
if (PROJECT_BINARY_DIR)
  set(BINARY_DIR ${PROJECT_BINARY_DIR})
else()
  set(BINARY_DIR ${CMAKE_BINARY_DIR})
endif()
set(BINARY_DIR_INCLUDE ${BINARY_DIR}/include)
set(BINARY_DIR_INCLUDE_GENERATED ${BINARY_DIR_INCLUDE}/generated)
set(BINARY_DIR_INCLUDE_GENERATED_H ${BINARY_DIR_INCLUDE_GENERATED})
file(MAKE_DIRECTORY ${BINARY_DIR_INCLUDE_GENERATED})
