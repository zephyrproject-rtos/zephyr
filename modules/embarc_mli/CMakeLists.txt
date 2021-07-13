# Copyright (c) 2021 Synopsys
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_EMBARC_MLI)
  if(NOT DEFINED ENV{METAWARE_ROOT})
    set(EMBARC_MLI_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
    set(EMBARC_MLI_INCLUDE_DIR ${EMBARC_MLI_DIR}/include ${EMBARC_MLI_DIR}/include/api)
    add_library(embarc_mli_lib STATIC IMPORTED GLOBAL)
    set_target_properties(embarc_mli_lib PROPERTIES IMPORTED_LOCATION ${EMBARC_MLI_DIR}/prebuilt/libmli.a)
    target_include_directories(embarc_mli_lib INTERFACE ${EMBARC_MLI_INCLUDE_DIR})
    add_dependencies(zephyr_interface embarc_mli_lib)
    zephyr_link_libraries(embarc_mli_lib)
  else()
    set(EMBARC_MLI_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
    set(EMBARC_MLI_INCLUDE_DIR ${EMBARC_MLI_DIR}/include ${EMBARC_MLI_DIR}/include/api)
    include(ExternalProject)
    ExternalProject_Add(
      embarc_mli_project           # Name for custom target
      PREFIX     ${EMBARC_MLI_DIR} # Root dir for entire project
      SOURCE_DIR ${EMBARC_MLI_DIR}
      BINARY_DIR ${EMBARC_MLI_DIR}/lib/make # This particular build system is invoked from the root
      CONFIGURE_COMMAND ""         # Skip configuring the project, e.g. with autoconf
      BUILD_COMMAND
      gmake -C ${EMBARC_MLI_DIR}
      TCF_FILE=${EMBARC_MLI_DIR}/hw/em9d.tcf
      BUILD_DIR=${ZEPHYR_BINARY_DIR}/embarc_mli
      LIBRARY_DIR=${ZEPHYR_BINARY_DIR}/embarc_mli/bin
      lib
      INSTALL_COMMAND ""
      BUILD_BYPRODUCTS ${ZEPHYR_BINARY_DIR}/embarc_mli/bin/libmli.a
      )
    # Create a wrapper CMake library that our app can link with
    add_library(embarc_mli_lib STATIC IMPORTED GLOBAL)
    set_target_properties(embarc_mli_lib PROPERTIES IMPORTED_LOCATION ${ZEPHYR_BINARY_DIR}/embarc_mli/bin/libmli.a)
    target_include_directories(embarc_mli_lib INTERFACE ${EMBARC_MLI_INCLUDE_DIR})
    add_dependencies(zephyr_interface embarc_mli_lib)
    zephyr_link_libraries(embarc_mli_lib)
  endif()
endif()
