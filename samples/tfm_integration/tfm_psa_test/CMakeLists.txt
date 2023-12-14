#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_psa_arch_test)

target_sources(app PRIVATE src/main.c)

get_target_property(TFM_BINARY_DIR           tfm TFM_BINARY_DIR)
get_target_property(TFM_NS_BIN_FILE          tfm TFM_NS_BIN_FILE)
get_target_property(TFM_NS_HEX_FILE          tfm TFM_NS_HEX_FILE)
get_target_property(TFM_NS_SIGNED_BIN_FILE   tfm TFM_NS_SIGNED_BIN_FILE)

get_target_property(TFM_TOOLCHAIN_PATH       tfm TFM_TOOLCHAIN_PATH)
get_target_property(TFM_TOOLCHAIN_PREFIX     tfm TFM_TOOLCHAIN_PREFIX)
get_target_property(TFM_TOOLCHAIN_NS_FILE    tfm TFM_TOOLCHAIN_NS_FILE)

set(TFM_TEST_REPO_PATH ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/../tf-m-tests)
set(TFM_PSA_ARCHTEST_REPO_PATH ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/../psa-arch-tests)

if (CONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION AND CONFIG_TFM_QCBOR_PATH STREQUAL "")
# TODO: Remove this when QCBOR licensing issues w/t_cose have been resolved,
# or only allow it when 'QCBOR_PATH' is set to a local path where QCBOR has
# been manually downloaded by the user before starting the build.
message(FATAL_ERROR "CONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION is not available "
  "with TF-M 2.0.0 due to licensing issues with a dependent library. This "
  "restriction will be removed once licensing issues have been resolved."
  )
endif()


set(TFM_TEST_DIR         "${TFM_TEST_REPO_PATH}/tests_psa_arch/spe/partitions")
set(PSA_ARCH_TESTS_CONFIG_FILE "${TFM_TEST_REPO_PATH}/tests_psa_arch/spe/config/config_test_psa_api.cmake")
if (CONFIG_TFM_PSA_TEST_CRYPTO)
set(TFM_PSA_TEST_SUITE CRYPTO)
elseif (CONFIG_TFM_PSA_TEST_PROTECTED_STORAGE)
set(TFM_PSA_TEST_SUITE PROTECTED_STORAGE)
elseif (CONFIG_TFM_PSA_TEST_INTERNAL_TRUSTED_STORAGE)
set(TFM_PSA_TEST_SUITE INTERNAL_TRUSTED_STORAGE)
elseif (CONFIG_TFM_PSA_TEST_STORAGE)
set(TFM_PSA_TEST_SUITE STORAGE)
elseif (CONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION)
set(TFM_PSA_TEST_SUITE INITIAL_ATTESTATION)
endif()

if (NOT DEFINED TFM_PSA_TEST_SUITE)
  message(FATAL_ERROR "Please define witch test suite to run:
    CONFIG_TFM_PSA_TEST_CRYPTO
    CONFIG_TFM_PSA_TEST_PROTECTED_STORAGE
    CONFIG_TFM_PSA_TEST_STORAGE
    CONFIG_TFM_PSA_TEST_INITIAL_ATTESTATION")
endif()
set(TEST_PSA_API "${TFM_PSA_TEST_SUITE}")

set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DPSA_ARCH_TESTS_PATH=${TFM_PSA_ARCHTEST_REPO_PATH}
)

set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DCONFIG_TFM_TEST_DIR=${TFM_TEST_DIR}
)

set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DCONFIG_PSA_ARCH_TESTS_CONFIG_FILE=${PSA_ARCH_TESTS_CONFIG_FILE}
)

set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DTEST_PSA_API=${TEST_PSA_API}
)

include(ExternalProject)

ExternalProject_Add(tfm_psa_arch_test_app
    SOURCE_DIR ${TFM_TEST_REPO_PATH}/tests_psa_arch
    BINARY_DIR ${PROJECT_BINARY_DIR}/tfm_ns
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND}
                -G ${CMAKE_GENERATOR}
                -S ${TFM_TEST_REPO_PATH}/tests_psa_arch
                -B ${PROJECT_BINARY_DIR}/tfm_ns
                -DCROSS_COMPILE=${TFM_TOOLCHAIN_PATH}/${TFM_TOOLCHAIN_PREFIX}
                -DPSA_TOOLCHAIN_FILE=${TFM_BINARY_DIR}/api_ns/cmake/${TFM_TOOLCHAIN_NS_FILE}
                -DCONFIG_SPE_PATH=${TFM_BINARY_DIR}/api_ns
                -DTFM_TOOLCHAIN_FILE=cmake/${TFM_TOOLCHAIN_NS_FILE}
                -DQCBOR_PATH${QCBOR_PATH_TYPE}=${CONFIG_TFM_QCBOR_PATH}
                -DCMAKE_BUILD_TYPE=RelWithDebInfo
                -DTEST_PSA_API=${TEST_PSA_API}
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

add_dependencies(app tfm_psa_arch_test_app)
