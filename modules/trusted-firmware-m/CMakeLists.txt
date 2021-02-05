# Copyright (c) 2019, 2020 Linaro
# Copyright (c) 2020, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# List of all partitions supported by TF-M
# Name must match name in 'trusted-firmware-m/config/config_default.cmake'
set(TFM_VALID_PARTITIONS
  TFM_PARTITION_PROTECTED_STORAGE
  TFM_PARTITION_INTERNAL_TRUSTED_STORAGE
  TFM_PARTITION_CRYPTO
  TFM_PARTITION_INITIAL_ATTESTATION
  TFM_PARTITION_PLATFORM
  TFM_PARTITION_AUDIT_LOG
  )

# Adds trusted-firmware-m as an external project.
# Also creates a target called 'tfm_api'
# which can be linked into the app.
#
# When called from a Zephyr module, the following input values can be provided
# to configure the TF-M build:
#
# BINARY_DIR: The location where the build outputs will be written
# BOARD: The string identifying the board target for TF-M (AN521, etc.)
# CMAKE_BUILD_TYPE: The TF-M build type to use, (Debug, Release, etc.)
# IPC: Build TFM IPC library. This library allows a non-secure application to
#      interface to secure domain using IPC.
# ISOLATION_LEVEL: The TF-M isolation level to use
# REGRESSION: Boolean if TF-M build includes building the TF-M regression tests
# BL2: Boolean if the TF-M build uses MCUboot. Default: True
# ENABLED_PARTITIONS: List of TFM partitions to enable.
#
# Example usage:
#
# trusted_firmware_build(BINARY_DIR ${CMAKE_BINARY_DIR}/tfm
#                        BOARD ${TFM_TARGET_PLATFORM}
#                        CMAKE_BUILD_TYPE Release
#                        IPC
#                        ISOLATION_LEVEL 2
#                        REGRESSION
#                        BL2
#                        BUILD_PROFILE profile_small
#                        ENABLED_PARTITIONS TFM_PARTITION_PLATFORM TFM_PARTITION_CRYPTO)
function(trusted_firmware_build)
  set(options IPC REGRESSION BL2)
  set(oneValueArgs BINARY_DIR BOARD ISOLATION_LEVEL CMAKE_BUILD_TYPE BUILD_PROFILE
    MCUBOOT_IMAGE_NUMBER)
  set(multiValueArgs ENABLED_PARTITIONS)
  cmake_parse_arguments(TFM "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  foreach(partition ${TFM_VALID_PARTITIONS})
    list(FIND TFM_ENABLED_PARTITIONS ${partition} idx)
    if (idx EQUAL -1)
      set(val "OFF")
    else()
      set(val "ON")
    endif()
    list(APPEND TFM_PARTITIONS_ARGS -D${partition}=${val})
  endforeach()

  if(TFM_IPC)
    set(TFM_IPC_ARG -DTFM_PSA_API=ON)
    # PSA API awareness for the Non-Secure application
    target_compile_definitions(app PRIVATE "TFM_PSA_API")
  endif()

  if(DEFINED TFM_ISOLATION_LEVEL)
    set(TFM_ISOLATION_LEVEL_ARG -DTFM_ISOLATION_LEVEL=${TFM_ISOLATION_LEVEL})
  endif()

  if(TFM_REGRESSION)
    set(TFM_REGRESSION_ARG -DTEST_S=ON)
  endif()

  if(DEFINED TFM_CMAKE_BUILD_TYPE)
    set(TFM_CMAKE_BUILD_TYPE_ARG -DCMAKE_BUILD_TYPE=${TFM_CMAKE_BUILD_TYPE})
  else()
    set(TFM_CMAKE_BUILD_TYPE_ARG -DCMAKE_BUILD_TYPE=RelWithDebInfo)
  endif()

  if(DEFINED TFM_BUILD_PROFILE)
    set(TFM_PROFILE_ARG -DTFM_PROFILE=${TFM_BUILD_PROFILE})
  endif()

  if(DEFINED TFM_MCUBOOT_IMAGE_NUMBER)
    set(MCUBOOT_IMAGE_NUM_ARG -DMCUBOOT_IMAGE_NUMBER=${TFM_MCUBOOT_IMAGE_NUMBER})
  endif()

  set(VENEERS_FILE ${TFM_BINARY_DIR}/secure_fw/s_veneers.o)
  set(PSA_API_NS_PATH ${TFM_BINARY_DIR}/interface/libpsa_api_ns.a)
  set(TFM_GENERATED_INCLUDES ${TFM_BINARY_DIR}/generated/interface/include)

  if(TFM_BL2)
    set(BL2_BIN_FILE ${TFM_BINARY_DIR}/bin/bl2.bin)
    set(BL2_HEX_FILE ${TFM_BINARY_DIR}/bin/bl2.hex)
  endif()
  set(TFM_S_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_s.bin)
  set(TFM_S_HEX_FILE ${TFM_BINARY_DIR}/bin/tfm_s.hex)
  set(TFM_NS_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_ns.bin)
  set(TFM_NS_HEX_FILE ${TFM_BINARY_DIR}/bin/tfm_ns.hex)
  set(TFM_S_SIGNED_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_s_signed.bin)
  set(TFM_NS_SIGNED_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_ns_signed.bin)
  set(TFM_S_NS_SIGNED_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_s_ns_signed.bin)

  set(BUILD_BYPRODUCTS
    ${VENEERS_FILE}
    ${PSA_API_NS_PATH}
    ${TFM_GENERATED_INCLUDES}/psa_manifest/sid.h
    ${BL2_BIN_FILE}
    ${BL2_HEX_FILE}
    ${TFM_S_BIN_FILE}
    ${TFM_S_HEX_FILE}
    ${TFM_NS_BIN_FILE}
    ${TFM_NS_HEX_FILE}
    ${TFM_S_SIGNED_BIN_FILE}
    ${TFM_NS_SIGNED_BIN_FILE}
    ${TFM_S_NS_SIGNED_BIN_FILE}
    )

  # Get the toolchain variant
  # TODO: Add support for cross-compile toolchain variant
  # TODO: Enforce GCC version check against TF-M compiler requirements
  if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "zephyr")
    set(TFM_TOOLCHAIN_FILE "trusted-firmware-m/toolchain_GNUARM.cmake")
    set(TFM_TOOLCHAIN_PREFIX "arm-zephyr-eabi")
    set(TFM_TOOLCHAIN_PATH ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin)
  elseif(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "gnuarmemb")
    set(TFM_TOOLCHAIN_FILE "trusted-firmware-m/toolchain_GNUARM.cmake")
    set(TFM_TOOLCHAIN_PREFIX "arm-none-eabi")
    set(TFM_TOOLCHAIN_PATH ${GNUARMEMB_TOOLCHAIN_PATH}/bin)
  else()
    message(FATAL_ERROR "Unsupported ZEPHYR_TOOLCHAIN_VARIANT: ${ZEPHYR_TOOLCHAIN_VARIANT}")
  endif()

  include(ExternalProject)

  ExternalProject_Add(
    tfm
    SOURCE_DIR ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/trusted-firmware-m
    BINARY_DIR ${TFM_BINARY_DIR}
    CMAKE_ARGS -DTFM_TOOLCHAIN_FILE=${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/${TFM_TOOLCHAIN_FILE}
               -DTFM_PLATFORM=${TFM_BOARD}
               -DCROSS_COMPILE=${TFM_TOOLCHAIN_PATH}/${TFM_TOOLCHAIN_PREFIX}
               ${TFM_CMAKE_BUILD_TYPE_ARG}
               -DBL2=${TFM_BL2}
               ${TFM_IPC_ARG}
               ${TFM_ISOLATION_LEVEL_ARG}
               ${TFM_REGRESSION_ARG}
               ${TFM_PROFILE_ARG}
               ${MCUBOOT_IMAGE_NUM_ARG}
               -DTFM_TEST_REPO_PATH=${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/tf-m-tests
               -DMCUBOOT_PATH=${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/../tfm-mcuboot
               ${TFM_PARTITIONS_ARGS}
    BUILD_ALWAYS True
    USES_TERMINAL_BUILD True
    BUILD_BYPRODUCTS ${BUILD_BYPRODUCTS}
  )

  # Set BL2 (MCUboot) executable file paths as target properties on 'tfm'
  # These files are produced by the TFM build system.
  if(TFM_BL2)
    set_target_properties(tfm PROPERTIES
      BL2_BIN_FILE ${BL2_BIN_FILE}
      BL2_HEX_FILE ${BL2_HEX_FILE}
      )
  endif()

  # Set TFM S/NS executable file paths as target properties on 'tfm'
  # These files are produced by the TFM build system.
  # Note that the Nonsecure FW is replaced by the Zephyr app in regular Zephyr
  # builds.
  set_target_properties(tfm PROPERTIES
    TFM_S_BIN_FILE ${TFM_S_BIN_FILE} # TFM Secure FW (unsigned)
    TFM_S_HEX_FILE ${TFM_S_HEX_FILE} # TFM Secure FW (unsigned)
    TFM_NS_BIN_FILE ${TFM_NS_BIN_FILE} # TFM Nonsecure FW (unsigned)
    TFM_NS_HEX_FILE ${TFM_NS_HEX_FILE} # TFM Nonsecure FW (unsigned)
    TFM_S_SIGNED_BIN_FILE ${TFM_S_SIGNED_BIN_FILE} # TFM Secure FW (signed)
    TFM_NS_SIGNED_BIN_FILE ${TFM_NS_SIGNED_BIN_FILE} # TFM Nonsecure FW (signed)
    TFM_S_NS_SIGNED_BIN_FILE ${TFM_S_NS_SIGNED_BIN_FILE} # Merged TFM Secure/Nonsecure FW (signed)
    )

  add_library(tfm_api
    ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/tf-m-tests/app/os_wrapper_cmsis_rtos_v2.c
  )

  target_include_directories(tfm_api
    PRIVATE
    ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/tf-m-tests/CMSIS/RTOS2/Include
    PUBLIC
    ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/trusted-firmware-m/interface/include
    INTERFACE
    ${TFM_GENERATED_INCLUDES}
  )

  target_link_libraries(tfm_api
    PUBLIC
    zephyr_interface
    INTERFACE
    ${PSA_API_NS_PATH}
    ${VENEERS_FILE}
    $<TARGET_FILE:tfm_api>
  )

  add_dependencies(tfm_api tfm)
endfunction()

if (CONFIG_BUILD_WITH_TFM)
  if (CONFIG_TFM_IPC)
    set(TFM_IPC_ARG IPC)
  endif()
  if (CONFIG_TFM_REGRESSION)
    set(TFM_REGRESSION_ARG REGRESSION)
  endif()
  if (CONFIG_TFM_BL2)
    set(TFM_BL2_ARG BL2)
  endif()
  if (CONFIG_TFM_ISOLATION_LEVEL)
    set(TFM_ISOLATION_LEVEL_ARG ISOLATION_LEVEL ${CONFIG_TFM_ISOLATION_LEVEL})
  endif()
  if (CONFIG_TFM_PROFILE)
    set(TFM_PROFILE_ARG BUILD_PROFILE ${CONFIG_TFM_PROFILE})
  endif()
  if (CONFIG_TFM_CMAKE_BUILD_TYPE_RELEASE)
    set(TFM_CMAKE_BUILD_TYPE "Release")
  elseif (CONFIG_TFM_CMAKE_BUILD_TYPE_MINSIZEREL)
    set(TFM_CMAKE_BUILD_TYPE "MinSizeRel")
  elseif (CONFIG_TFM_CMAKE_BUILD_TYPE_DEBUG)
    set(TFM_CMAKE_BUILD_TYPE "Debug")
  elseif (CONFIG_TFM_CMAKE_BUILD_TYPE_RELWITHDEBINFO)
    set(TFM_CMAKE_BUILD_TYPE "RelWithDebInfo")
  endif()
  if (DEFINED CONFIG_TFM_MCUBOOT_IMAGE_NUMBER)
    set(TFM_IMAGE_NUMBER_ARG
        MCUBOOT_IMAGE_NUMBER ${CONFIG_TFM_MCUBOOT_IMAGE_NUMBER})
  endif()

  # Enable TFM partitions as specified in Kconfig
  foreach(partition ${TFM_VALID_PARTITIONS})
    if (CONFIG_${partition})
      list(APPEND TFM_ENABLED_PARTITIONS_ARG ${partition})
    endif()
  endforeach()

  trusted_firmware_build(
    BINARY_DIR ${CMAKE_BINARY_DIR}/tfm
    BOARD ${CONFIG_TFM_BOARD}
    ${TFM_ISOLATION_LEVEL_ARG}
    ${TFM_PROFILE_ARG}
    ${TFM_IMAGE_NUMBER_ARG}
    ${TFM_BL2_ARG}
    ${TFM_IPC_ARG}
    ${TFM_REGRESSION_ARG}
    ENABLED_PARTITIONS ${TFM_ENABLED_PARTITIONS_ARG}
    CMAKE_BUILD_TYPE ${TFM_CMAKE_BUILD_TYPE}
  )

  zephyr_link_libraries(tfm_api)

  # Set default image versions if not defined elsewhere
  if (NOT DEFINED TFM_IMAGE_VERSION_S)
    set(TFM_IMAGE_VERSION_S 0.0.0+0)
  endif()

  if (NOT DEFINED TFM_IMAGE_VERSION_NS)
    set(TFM_IMAGE_VERSION_NS 0.0.0+0)
  endif()

  if (CONFIG_TFM_BL2)
    set(PREPROCESSED_FILE_S "${CMAKE_BINARY_DIR}/tfm/bl2/ext/mcuboot/CMakeFiles/signing_layout_s.dir/signing_layout_s.o")
    set(PREPROCESSED_FILE_NS "${CMAKE_BINARY_DIR}/tfm/bl2/ext/mcuboot/CMakeFiles/signing_layout_ns.dir/signing_layout_ns.o")
    set(TFM_MCUBOOT_DIR "${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/trusted-firmware-m/bl2/ext/mcuboot")
  endif()

  # Configure which format (full or hash) to include the public key in
  # the image manifest
  if(NOT DEFINED TFM_PUBLIC_KEY_FORMAT)
    set(TFM_PUBLIC_KEY_FORMAT "full")
  endif()

  function(tfm_sign OUT_ARG SUFFIX PAD INPUT_FILE OUTPUT_FILE)
    if(PAD)
      set(pad_args --pad --pad-header)
    endif()
    set (${OUT_ARG}
      ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/wrapper/wrapper.py
      --layout ${PREPROCESSED_FILE_${SUFFIX}}
      -k ${CONFIG_TFM_KEY_FILE_${SUFFIX}}
      --public-key-format ${TFM_PUBLIC_KEY_FORMAT}
      --align 1
      -v ${TFM_IMAGE_VERSION_${SUFFIX}}
      ${pad_args}
      ${ADD_${SUFFIX}_IMAGE_MIN_VER}
      -s auto
      -H ${CONFIG_ROM_START_OFFSET}
      ${INPUT_FILE}
      ${OUTPUT_FILE}
      PARENT_SCOPE)
  endfunction()

  set(MERGED_FILE ${CMAKE_BINARY_DIR}/tfm_merged.hex)
  set(S_NS_FILE ${CMAKE_BINARY_DIR}/tfm_s_zephyr_ns.hex)
  set(S_NS_SIGNED_FILE ${CMAKE_BINARY_DIR}/tfm_s_zephyr_ns_signed.hex)
  set(NS_SIGNED_FILE ${CMAKE_BINARY_DIR}/zephyr_ns_signed.hex)
  set(S_SIGNED_FILE ${CMAKE_BINARY_DIR}/tfm_s_signed.hex)

  if (NOT CONFIG_TFM_BL2)
    # Merge tfm_s and zephyr (NS) image to a single binary.
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/mergehex.py
        -o ${MERGED_FILE}
        $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>
        ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}
    )

    set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
      ${MERGED_FILE}
    )

  elseif(CONFIG_TFM_MCUBOOT_IMAGE_NUMBER STREQUAL "1")
    tfm_sign(sign_cmd NS TRUE ${S_NS_FILE} ${S_NS_SIGNED_FILE})

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/mergehex.py
        -o ${S_NS_FILE}
        $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>
        ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}

      COMMAND ${sign_cmd}

      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/mergehex.py
        -o ${MERGED_FILE}
        $<TARGET_PROPERTY:tfm,BL2_HEX_FILE>
        ${S_NS_SIGNED_FILE}
    )

    set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
      ${S_NS_FILE}
      ${S_NS_SIGNED_FILE}
      ${MERGED_FILE}
    )

  else()
    tfm_sign(sign_cmd_ns NS FALSE ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME} ${NS_SIGNED_FILE})
    tfm_sign(sign_cmd_s S TRUE $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE> ${S_SIGNED_FILE})

    #Create and sign for concatenated binary image, should align with the TF-M BL2
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${sign_cmd_ns}
      COMMAND ${sign_cmd_s}

      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/mergehex.py
        -o ${MERGED_FILE}
        $<TARGET_PROPERTY:tfm,BL2_HEX_FILE>
        ${S_SIGNED_FILE}
        ${NS_SIGNED_FILE}
    )

    set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
      ${S_SIGNED_FILE}
      ${NS_SIGNED_FILE}
      ${MERGED_FILE}
    )
  endif()
endif()
