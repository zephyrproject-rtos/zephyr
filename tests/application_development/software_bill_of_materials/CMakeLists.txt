# Copyright (c) 2024 Basalte bv
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Pull two extra Zephyr modules into the build that we fully control so they are reflected in the
# generated SBOM and we can verify various properties (license detection, source presence, etc.).
# One of them (sbom_used_module) has code that is actually compiled and called from the application,
# so its source is analyzed; the other (sbom_unused_module) is declared but builds nothing, so it
# appears only as an empty source package and a dependency entry. Both modules live in-tree under
# this application directory; 'west spdx' still attributes their sources to the owning module.
list(APPEND EXTRA_ZEPHYR_MODULES
  "${CMAKE_CURRENT_SOURCE_DIR}/used_module"
  "${CMAKE_CURRENT_SOURCE_DIR}/unused_module"
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sbom_spdx)
target_sources(app PRIVATE src/main.c)

enable_testing()
include(CTest)

set(
  SPDX2_FILES
  app.spdx
  zephyr.spdx
  build.spdx
  modules-deps.spdx
)

set(
  SPDX3_FILES
  app.jsonld
  zephyr.jsonld
  build.jsonld
  modules-deps.jsonld
)

set(SPDX_VERSIONS_TO_TEST "2.2" "2.3" "3.0")

function(add_spdx2_validation_tests spdx_version version_suffix)
  foreach(spdx_file ${SPDX2_FILES})
    # Smoke test: check spdx_file is a valid SPDX 2.x document as per pyspdxtools
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

  add_test(
    NAME spdx_content_validate_${version_suffix}
    COMMAND ${PYTHON_EXECUTABLE} -m pytest
      ${CMAKE_CURRENT_SOURCE_DIR}/verify_spdx_content.py
      --build-dir ${CMAKE_BINARY_DIR}
      --spdx-version ${spdx_version}
      --source-dir ${CMAKE_CURRENT_SOURCE_DIR}
      -v
  )

  set_tests_properties(
    spdx_content_validate_${version_suffix}
    PROPERTIES DEPENDS spdx_gen_${version_suffix}
  )
endfunction()

function(add_spdx3_validation_tests version_suffix)
  foreach(spdx_file ${SPDX3_FILES})
    # Smoke test: check spdx_file is a valid SPDX 3.x JSON-LD document
    get_filename_component(basename ${spdx_file} NAME_WE)

    add_test(
      NAME spdx_validate_${basename}_${version_suffix}
      COMMAND spdx3-validate --quiet --spdx-version auto
                             --json ${CMAKE_BINARY_DIR}/spdx/${spdx_file}
    )

    set_tests_properties(
      spdx_validate_${basename}_${version_suffix}
      PROPERTIES DEPENDS spdx_gen_${version_suffix}
    )
  endforeach()
endfunction()

foreach(spdx_version ${SPDX_VERSIONS_TO_TEST})
  string(REPLACE "." "_" version_suffix ${spdx_version})

  add_test(
    NAME spdx_gen_${version_suffix}
    COMMAND west spdx -d ${CMAKE_BINARY_DIR} --spdx-version ${spdx_version}
  )
  set_tests_properties(
    spdx_gen_${version_suffix}
    PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  if(spdx_version VERSION_LESS "3.0")
    add_spdx2_validation_tests(${spdx_version} ${version_suffix})
  else()
    add_spdx3_validation_tests(${version_suffix})
  endif()
endforeach()
