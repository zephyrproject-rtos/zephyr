# Copyright (c) 2024 Basalte bv
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sbom_spdx)
target_sources(app PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)

enable_testing()
include(CTest)

set(
  SPDX_FILES
  app.spdx
  zephyr.spdx
  build.spdx
  modules-deps.spdx
)

set(SPDX_VERSIONS_TO_TEST "2.2" "2.3")

foreach(spdx_version ${SPDX_VERSIONS_TO_TEST})
  string(REPLACE "." "_" version_suffix ${spdx_version})

  add_test(
    NAME spdx_gen_${version_suffix}
    COMMAND west spdx -d ${CMAKE_BINARY_DIR} --spdx-version ${spdx_version}
  )

  foreach(spdx_file ${SPDX_FILES})
    get_filename_component(basename ${spdx_file} NAME_WE)

    add_test(
      NAME spdx_validate_${basename}_${version_suffix}
      COMMAND pyspdxtools -i ${CMAKE_BINARY_DIR}/spdx/${spdx_file}
    )

    set_tests_properties(
      spdx_validate_${basename}_${version_suffix}
      PROPERTIES DEPENDS spdx_gen_${version_suffix}
    )
  endforeach()
endforeach()
