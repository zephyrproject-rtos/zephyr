# Copyright (c) 2018-2020 O.S.Systems
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# This application has its own Kconfig options.
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(updatehub)

target_sources(app PRIVATE src/main.c)

if(CONFIG_UPDATEHUB_DTLS)
  get_filename_component(root_ca ${CONFIG_UPDATEHUB_SAMPLE_ROOT_CA_FILE}
    ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  )
  generate_inc_file_for_target(app
    ${root_ca}
    ${ZEPHYR_BINARY_DIR}/include/generated/updatehub-root-ca.pem.inc
  )
endif()
