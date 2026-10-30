# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uf2)

include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)

# CONFIG_APP_UF2_FAMILY_ID is either a number or a family name known to
# uf2conv.py, the application needs the number.
set(uf2_family "${CONFIG_APP_UF2_FAMILY_ID}")
if(NOT uf2_family MATCHES "^0[xX][0-9a-fA-F]+$")
  string(TOUPPER "${uf2_family}" uf2_family_name)
  file(READ ${ZEPHYR_BASE}/scripts/build/uf2families.json uf2_families)
  string(JSON uf2_families_len LENGTH "${uf2_families}")
  math(EXPR uf2_families_last "${uf2_families_len} - 1")

  set(uf2_family "")
  foreach(i RANGE ${uf2_families_last})
    string(JSON short_name GET "${uf2_families}" ${i} short_name)
    if(short_name STREQUAL uf2_family_name)
      string(JSON uf2_family GET "${uf2_families}" ${i} id)
      break()
    endif()
  endforeach()
endif()

if(uf2_family STREQUAL "")
  message(FATAL_ERROR "Unknown UF2 family '${CONFIG_APP_UF2_FAMILY_ID}', "
                      "set CONFIG_APP_UF2_FAMILY_ID")
endif()

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_compile_definitions(app PRIVATE UF2_FAMILY_ID=${uf2_family}U)
