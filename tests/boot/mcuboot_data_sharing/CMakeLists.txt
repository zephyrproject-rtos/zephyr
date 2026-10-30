#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mcuboot_recovery_retention)

if(NOT (DEFINED SYSBUILD))
  message(FATAL_ERROR "This test must be built with sysbuild.")
endif()

FILE(GLOB app_sources
  src/*.c
)

zephyr_get(sysbuild_binary_dir VAR APPLICATION_BINARY_DIR SYSBUILD GLOBAL)
import_kconfig(SB_CONFIG_BOOT_SIGNATURE_TYPE_* "${sysbuild_binary_dir}/zephyr/.config")

if(SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256)
  target_compile_definitions(app PRIVATE EXPECTED_BOOT_SIGNATURE=MCUBOOT_SIGNATURE_TYPE_ECDSA_P256)
elseif(SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA)
  target_compile_definitions(app PRIVATE EXPECTED_BOOT_SIGNATURE=MCUBOOT_SIGNATURE_TYPE_RSA)
elseif(SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE)
  target_compile_definitions(app PRIVATE EXPECTED_BOOT_SIGNATURE=MCUBOOT_SIGNATURE_TYPE_NONE)
elseif(SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519)
  target_compile_definitions(app PRIVATE EXPECTED_BOOT_SIGNATURE=MCUBOOT_SIGNATURE_TYPE_ED25519)
endif()

target_sources(app PRIVATE ${app_sources})

# Get MCUboot version from the VERSION file in the repository and create a local output header
# version file so that it can be compared against in the test
set(VERSION_FILE ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/VERSION)
set(VERSION_TYPE MCUBOOT)
set(BUILD_VERSION_NAME MCUBOOT_BUILD_VERSION)
include(${ZEPHYR_BASE}/cmake/modules/version.cmake)
file(READ ${ZEPHYR_BASE}/version.h.in version_content)
string(CONFIGURE "${version_content}" version_content)
string(CONFIGURE "${version_content}" version_content)
file(
  WRITE
  ${CMAKE_CURRENT_BINARY_DIR}/zephyr/include/generated/zephyr/mcuboot_version.h
  "${version_content}"
)
