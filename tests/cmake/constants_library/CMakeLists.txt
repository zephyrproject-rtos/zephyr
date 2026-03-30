# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Register constants libraries while still in Zephyr's project scope
# so that generated headers land under build/zephyr/include/generated/.
#
# Dependency graph:
#   B: standalone
#   A -> B
#   C -> B
#   D -> A, B

zephyr_constants_library(
  NAME test_constants_a
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/constants_a.c
  DEPENDS test_constants_b
)

zephyr_constants_library(
  NAME test_constants_b
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/constants_b.c
)

zephyr_constants_library(
  NAME test_constants_c
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/constants_c.c
  DEPENDS test_constants_b
)

zephyr_constants_library(
  NAME test_constants_d
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/constants_d.c
  DEPENDS test_constants_a test_constants_b
)

project(constants_library)

target_sources(app PRIVATE src/main.c)
