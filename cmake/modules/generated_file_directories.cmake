# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

# This file creates locations in the build directory
# for placing generated files.
#
# Outcome:
# - BINARY_DIR_INCLUDE is set to ${PROJECT_BINARY_DIR}/include
# - BINARY_DIR_INCLUDE_GENERATED is set to ${BINARY_DIR_INCLUDE}/generated/zephyr
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

set(BINARY_DIR_INCLUDE ${PROJECT_BINARY_DIR}/include)
set(BINARY_DIR_INCLUDE_GENERATED ${BINARY_DIR_INCLUDE}/generated/zephyr)
file(MAKE_DIRECTORY ${BINARY_DIR_INCLUDE_GENERATED})
