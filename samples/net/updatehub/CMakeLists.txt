# Copyright (c) 2018 O.S.Systems
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# This application has its own Kconfig options.
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/lib/updatehub/include)
target_sources(app PRIVATE src/main.c)

if(CONFIG_UPDATEHUB_DTLS)
    foreach(inc_file
            servercert.der
            privkey.der
            )
        generate_inc_file_for_target(
            ${ZEPHYR_CURRENT_LIBRARY}
            src/certificates/${inc_file}
            ${gen_dir}/${inc_file}.inc
            )
    endforeach()
endif()
