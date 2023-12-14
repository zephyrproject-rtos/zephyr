#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_regression_test)

target_sources(app PRIVATE src/main.c)

get_target_property(TFM_BINARY_DIR           tfm TFM_BINARY_DIR)
get_target_property(TFM_NS_BIN_FILE          tfm TFM_NS_BIN_FILE)
get_target_property(TFM_NS_HEX_FILE          tfm TFM_NS_HEX_FILE)
get_target_property(TFM_NS_SIGNED_BIN_FILE   tfm TFM_NS_SIGNED_BIN_FILE)

set(TFM_TEST_REPO_PATH ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/../tf-m-tests)

set(TFM_TEST_DIR         "${TFM_TEST_REPO_PATH}/tests_reg/test/secure_regression")
set(TFM_TEST_CONFIG_FILE "${TFM_TEST_REPO_PATH}/tests_reg/test/config/config.cmake")

set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DCONFIG_TFM_TEST_DIR=${TFM_TEST_DIR}
)

set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DCONFIG_TFM_TEST_CONFIG_FILE=${TFM_TEST_CONFIG_FILE}
)

include(ExternalProject)

ExternalProject_Add(tfm_regression_test_app
    SOURCE_DIR ${TFM_TEST_REPO_PATH}/tests_reg
    BINARY_DIR ${PROJECT_BINARY_DIR}/tfm_ns
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND}
                -G ${CMAKE_GENERATOR}
                -S ${TFM_TEST_REPO_PATH}/tests_reg
                -B ${PROJECT_BINARY_DIR}/tfm_ns
                -DCONFIG_SPE_PATH=${TFM_BINARY_DIR}/api_ns
                -DTFM_TOOLCHAIN_FILE=cmake/toolchain_ns_GNUARM.cmake
                -DQCBOR_PATH${QCBOR_PATH_TYPE}=${CONFIG_TFM_QCBOR_PATH}
                -DCMAKE_BUILD_TYPE=RelWithDebInfo
    BUILD_COMMAND ${CMAKE_COMMAND} --build .
    INSTALL_COMMAND ""
    BUILD_ALWAYS True
    USES_TERMINAL_BUILD True
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/tfm_ns
    DEPENDS tfm
    BUILD_BYPRODUCTS
        ${TFM_NS_HEX_FILE}
        ${TFM_NS_BIN_FILE}
        ${TFM_NS_SIGNED_BIN_FILE}
)

add_dependencies(app tfm_regression_test_app)
