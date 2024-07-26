# Copyright (c) 2019 - 2023 Linaro
# Copyright (c) 2020 - 2023 Nordic Semiconductor ASA
# Copyright 2023-2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

# List of all partitions supported by TF-M
# Name must match name in 'trusted-firmware-m/tools/tfm_manifest_list.yaml'
set(TFM_VALID_PARTITIONS
  TFM_PARTITION_NS_AGENT_MAILBOX
  TFM_PARTITION_PROTECTED_STORAGE
  TFM_PARTITION_INTERNAL_TRUSTED_STORAGE
  TFM_PARTITION_CRYPTO
  TFM_PARTITION_PLATFORM
  TFM_PARTITION_INITIAL_ATTESTATION
  TFM_PARTITION_FIRMWARE_UPDATE
  )

if (CONFIG_BUILD_WITH_TFM)
  # PSA API awareness for the Non-Secure application
  target_compile_definitions(app PRIVATE "TFM_PSA_API")
  if (CONFIG_TFM_SFN)
    list(APPEND TFM_CMAKE_ARGS -DCONFIG_TFM_SPM_BACKEND="SFN")
  else() # CONFIG_TFM_IPC
    list(APPEND TFM_CMAKE_ARGS -DCONFIG_TFM_SPM_BACKEND="IPC")
  endif()
  if (CONFIG_TFM_REGRESSION_S)
    list(APPEND TFM_CMAKE_ARGS -DTEST_S=ON)
    list(APPEND TFM_CMAKE_ARGS -DTFM_S_REG_TEST:BOOL=ON)
  endif()
  if (CONFIG_TFM_REGRESSION_NS)
    list(APPEND TFM_CMAKE_ARGS -DTEST_NS=ON)
    list(APPEND TFM_CMAKE_ARGS -DTFM_NS_REG_TEST:BOOL=ON)
  endif()
  if (CONFIG_TFM_BL2)
    list(APPEND TFM_CMAKE_ARGS -DBL2=TRUE)
    list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_IMAGE_VERSION_S=${CONFIG_TFM_IMAGE_VERSION_S})
    list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_IMAGE_VERSION_NS=${CONFIG_TFM_IMAGE_VERSION_NS})
    list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_SIGNATURE_TYPE=${CONFIG_TFM_MCUBOOT_SIGNATURE_TYPE})

    # TF-M's config/check_config.cmake requires MCUBOOT_BUILTIN_KEY=OFF for RSA
    # and MCUBOOT_USE_PSA_CRYPTO for EC-P. The others are dependencies needed
    # for either the build or the boot to succeed.
    if (${CONFIG_TFM_MCUBOOT_SIGNATURE_TYPE} MATCHES "^RSA")
      list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_BUILTIN_KEY=OFF)
      list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_HW_KEY=ON)
    elseif (${CONFIG_TFM_MCUBOOT_SIGNATURE_TYPE} MATCHES "^EC-P")
      list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_USE_PSA_CRYPTO=ON)
      list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_BUILTIN_KEY=ON)
      list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_HW_KEY=OFF)
    endif()

    foreach(SUFFIX IN ITEMS "S" "NS")
      string(CONFIGURE ${CONFIG_TFM_KEY_FILE_${SUFFIX}} CONFIG_TFM_KEY_FILE_${SUFFIX})
      list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_KEY_${SUFFIX}=${CONFIG_TFM_KEY_FILE_${SUFFIX}})
    endforeach()

  else()
    list(APPEND TFM_CMAKE_ARGS -DBL2=FALSE)
  endif()
  if (CONFIG_TFM_ISOLATION_LEVEL)
    list(APPEND TFM_CMAKE_ARGS -DTFM_ISOLATION_LEVEL=${CONFIG_TFM_ISOLATION_LEVEL})
  endif()
  if (CONFIG_TFM_ITS_NUM_ASSETS_OVERRIDE)
    list(APPEND TFM_CMAKE_ARGS -DITS_NUM_ASSETS=${CONFIG_TFM_ITS_NUM_ASSETS})
  endif()
  if (CONFIG_TFM_ITS_MAX_ASSET_SIZE_OVERRIDE)
    list(APPEND TFM_CMAKE_ARGS -DITS_MAX_ASSET_SIZE=${CONFIG_TFM_ITS_MAX_ASSET_SIZE})
  endif()
  if (CONFIG_TFM_PROFILE)
    list(APPEND TFM_CMAKE_ARGS -DTFM_PROFILE=${CONFIG_TFM_PROFILE})
  endif()
  if (CONFIG_TFM_CMAKE_BUILD_TYPE_RELEASE)
    set(TFM_CMAKE_BUILD_TYPE "Release")
  elseif (CONFIG_TFM_CMAKE_BUILD_TYPE_MINSIZEREL)
    set(TFM_CMAKE_BUILD_TYPE "MinSizeRel")
  elseif (CONFIG_TFM_CMAKE_BUILD_TYPE_DEBUG)
    set(TFM_CMAKE_BUILD_TYPE "Debug")
  else ()
    set(TFM_CMAKE_BUILD_TYPE "RelWithDebInfo")
  endif()
  if (DEFINED CONFIG_TFM_MCUBOOT_IMAGE_NUMBER)
    list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_IMAGE_NUMBER=${CONFIG_TFM_MCUBOOT_IMAGE_NUMBER})
  endif()

  if (CONFIG_TFM_DUMMY_PROVISIONING)
    list(APPEND TFM_CMAKE_ARGS -DTFM_DUMMY_PROVISIONING=ON)
  else()
    list(APPEND TFM_CMAKE_ARGS -DTFM_DUMMY_PROVISIONING=OFF)
  endif()

  if (CONFIG_TFM_EXCEPTION_INFO_DUMP)
    list(APPEND TFM_CMAKE_ARGS -DTFM_EXCEPTION_INFO_DUMP=ON)
  else()
    list(APPEND TFM_CMAKE_ARGS -DTFM_EXCEPTION_INFO_DUMP=OFF)
  endif()

  if (CONFIG_TFM_BL2)
    if (CONFIG_TFM_BL2_LOG_LEVEL_DEBUG)
      set(TFM_BL2_LOG_LEVEL "DEBUG")
    elseif (CONFIG_TFM_BL2_LOG_LEVEL_INFO)
      set(TFM_BL2_LOG_LEVEL "INFO")
    elseif (CONFIG_TFM_BL2_LOG_LEVEL_WARNING)
      set(TFM_BL2_LOG_LEVEL "WARNING")
    elseif (CONFIG_TFM_BL2_LOG_LEVEL_ERROR)
      set(TFM_BL2_LOG_LEVEL "ERROR")
    elseif (CONFIG_TFM_BL2_LOG_LEVEL_OFF OR CONFIG_TFM_LOG_LEVEL_SILENCE)
      set(TFM_BL2_LOG_LEVEL "OFF")
    endif()

    if (DEFINED TFM_BL2_LOG_LEVEL)
      # BL2 uses MCUBOOT_LOG_LEVEL configuration
      list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_LOG_LEVEL=${TFM_BL2_LOG_LEVEL})
    endif()
  endif()

  if (CONFIG_TFM_PARTITION_LOG_LEVEL_DEBUG)
    set(TFM_PARTITION_LOG_LEVEL "TFM_PARTITION_LOG_LEVEL_DEBUG")
  elseif (CONFIG_TFM_PARTITION_LOG_LEVEL_INFO)
    set(TFM_PARTITION_LOG_LEVEL "TFM_PARTITION_LOG_LEVEL_INFO")
  elseif (CONFIG_TFM_PARTITION_LOG_LEVEL_ERROR)
    set(TFM_PARTITION_LOG_LEVEL "TFM_PARTITION_LOG_LEVEL_ERROR")
  elseif (CONFIG_TFM_PARTITION_LOG_LEVEL_SILENCE OR CONFIG_TFM_LOG_LEVEL_SILENCE)
    set(TFM_PARTITION_LOG_LEVEL "TFM_PARTITION_LOG_LEVEL_SILENCE")
  endif()

  if (DEFINED TFM_PARTITION_LOG_LEVEL)
    list(APPEND TFM_CMAKE_ARGS -DTFM_PARTITION_LOG_LEVEL=${TFM_PARTITION_LOG_LEVEL})
  endif()

  if (CONFIG_TFM_SPM_LOG_LEVEL_DEBUG)
    set(TFM_SPM_LOG_LEVEL "TFM_SPM_LOG_LEVEL_DEBUG")
  elseif (CONFIG_TFM_SPM_LOG_LEVEL_INFO)
    set(TFM_SPM_LOG_LEVEL "TFM_SPM_LOG_LEVEL_INFO")
  elseif (CONFIG_TFM_SPM_LOG_LEVEL_ERROR)
    set(TFM_SPM_LOG_LEVEL "TFM_SPM_LOG_LEVEL_ERROR")
  elseif (CONFIG_TFM_SPM_LOG_LEVEL_SILENCE OR CONFIG_TFM_LOG_LEVEL_SILENCE)
    set(TFM_SPM_LOG_LEVEL "TFM_SPM_LOG_LEVEL_SILENCE")
  endif()

  if (DEFINED TFM_SPM_LOG_LEVEL)
    list(APPEND TFM_CMAKE_ARGS -DTFM_SPM_LOG_LEVEL=${TFM_SPM_LOG_LEVEL})
  endif()

  # Enable TFM partitions as specified in Kconfig
  foreach(partition ${TFM_VALID_PARTITIONS})
    if (CONFIG_${partition})
      # list(APPEND TFM_ENABLED_PARTITIONS_ARG ${partition})
      set(val "ON")
    else()
      set(val "OFF")
    endif()
    list(APPEND TFM_CMAKE_ARGS -D${partition}=${val})
  endforeach()

  set(TFM_BINARY_DIR ${CMAKE_BINARY_DIR}/tfm)

  set(TFM_INTERFACE_SOURCE_DIR   ${TFM_BINARY_DIR}/api_ns/interface/src)
  set(TFM_INTERFACE_INCLUDE_DIR  ${TFM_BINARY_DIR}/api_ns/interface/include)
  set(TFM_INTERFACE_LIB_DIR      ${TFM_BINARY_DIR}/api_ns/interface/lib)

  if(CONFIG_TFM_BL2)
    set(BL2_ELF_FILE ${TFM_BINARY_DIR}/bin/bl2.elf)
    set(BL2_BIN_FILE ${TFM_BINARY_DIR}/bin/bl2.bin)
    set(BL2_HEX_FILE ${TFM_BINARY_DIR}/bin/bl2.hex)
  endif()
  set(TFM_S_ELF_FILE ${TFM_BINARY_DIR}/bin/tfm_s.elf)
  set(TFM_S_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_s.bin)
  set(TFM_S_HEX_FILE ${TFM_BINARY_DIR}/bin/tfm_s.hex)
  set(TFM_NS_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_ns.bin)
  set(TFM_NS_HEX_FILE ${CMAKE_BINARY_DIR}/tfm_ns/bin/tfm_ns.hex)
  set(TFM_S_SIGNED_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_s_signed.bin)
  set(TFM_NS_SIGNED_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_ns_signed.bin)
  set(TFM_S_NS_SIGNED_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_s_ns_signed.bin)

  set(BUILD_BYPRODUCTS
    ${PSA_TEST_VAL_FILE}
    ${PSA_TEST_PAL_FILE}
    ${PSA_TEST_COMBINE_FILE}
    ${BL2_ELF_FILE}
    ${BL2_BIN_FILE}
    ${BL2_HEX_FILE}
    ${TFM_S_ELF_FILE}
    ${TFM_S_BIN_FILE}
    ${TFM_S_HEX_FILE}
    ${TFM_S_SIGNED_BIN_FILE}
    ${TFM_S_NS_SIGNED_BIN_FILE}

    ${TFM_INTERFACE_LIB_DIR}/s_veneers.o

    ${TFM_INTERFACE_SOURCE_DIR}/tfm_attest_api.c
    ${TFM_INTERFACE_SOURCE_DIR}/tfm_crypto_api.c
    ${TFM_INTERFACE_SOURCE_DIR}/tfm_fwu_api.c
    ${TFM_INTERFACE_SOURCE_DIR}/tfm_its_api.c
    ${TFM_INTERFACE_SOURCE_DIR}/tfm_platform_api.c
    ${TFM_INTERFACE_SOURCE_DIR}/tfm_ps_api.c
    ${TFM_INTERFACE_SOURCE_DIR}/tfm_tz_psa_ns_api.c

    # Specific to nordic platform
    ${TFM_INTERFACE_SOURCE_DIR}/tfm_ioctl_core_ns_api.c

    # Specific to nxp-rw61x platform
    ${TFM_INTERFACE_SOURCE_DIR}/tfm_lds_api.c
    )

  # Get the toolchain variant
  # TODO: Add support for cross-compile toolchain variant
  # TODO: Enforce GCC version check against TF-M compiler requirements
  if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "zephyr")
    set(TFM_TOOLCHAIN_FILE "toolchain_GNUARM.cmake")
    set(TFM_TOOLCHAIN_PREFIX "arm-zephyr-eabi")
    set(TFM_TOOLCHAIN_PATH ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin)
  elseif(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "gnuarmemb")
    set(TFM_TOOLCHAIN_FILE "toolchain_GNUARM.cmake")
    set(TFM_TOOLCHAIN_PREFIX "arm-none-eabi")
    set(TFM_TOOLCHAIN_PATH ${GNUARMEMB_TOOLCHAIN_PATH}/bin)
  elseif(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "xtools")
    set(TFM_TOOLCHAIN_FILE "toolchain_GNUARM.cmake")
    set(TFM_TOOLCHAIN_PREFIX "arm-zephyr-eabi")
    set(TFM_TOOLCHAIN_PATH ${XTOOLS_TOOLCHAIN_PATH}/arm-zephyr-eabi/bin)
  else()
    message(FATAL_ERROR "Unsupported ZEPHYR_TOOLCHAIN_VARIANT: ${ZEPHYR_TOOLCHAIN_VARIANT}")
  endif()

  if (CONFIG_TFM_PARTITION_INITIAL_ATTESTATION AND CONFIG_TFM_QCBOR_PATH STREQUAL "")
    # TODO: Remove this when QCBOR licensing issues w/t_cose have been resolved,
    # or only allow it when 'QCBOR_PATH' is set to a local path where QCBOR has
    # been manually downloaded by the user before starting the build.
    message(FATAL_ERROR "CONFIG_TFM_PARTITION_INITIAL_ATTESTATION is not available "
      "with TF-M due to licensing issues with a dependent library. This "
      "restriction will be removed once licensing issues have been resolved."
      )
  endif()

  string(REPLACE "toolchain" "toolchain_ns" TFM_TOOLCHAIN_NS_FILE ${TFM_TOOLCHAIN_FILE})

  if(CONFIG_BOARD_LPCXPRESSO55S69_LPC55S69_CPU0_NS)
    # Supply path to NXP HAL sources used for TF-M build
    set(TFM_PLATFORM_NXP_HAL_FILE_PATH ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/platform/ext/target/nxp/)
    list(APPEND TFM_CMAKE_ARGS -DTFM_PLATFORM_NXP_HAL_FILE_PATH=${TFM_PLATFORM_NXP_HAL_FILE_PATH})
  endif()

  if(CONFIG_BOARD_RD_RW612_BGA)
    # Supply path to NXP HAL sources used for TF-M build
    list(APPEND TFM_CMAKE_ARGS -DZEPHYR_HAL_NXP_MODULE_DIR=${ZEPHYR_HAL_NXP_MODULE_DIR})
    # Supply path to NXP HAL sources used for TF-M build
    set(TFM_PLATFORM_NXP_HAL_FILE_PATH ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/platform/ext/target/nxp/)
    list(APPEND TFM_CMAKE_ARGS -DTFM_PLATFORM_NXP_HAL_FILE_PATH=${TFM_PLATFORM_NXP_HAL_FILE_PATH})
    # Disable watchdog support on this platform for tfm_psa_tests
    if(TFM_PSA_TEST_SUITE)
      list(APPEND TFM_CMAKE_ARGS -DWATCHDOG_AVAILABLE=OFF)
    endif()

    # Platform specific TF-M cmake flags
    set(TFM_CMAKE_OPTIONS_PLATFORM_FLAG
      USE_TFM_EL2GO_DATA_IMPORT_REGION
      USE_TFM_WIFI_FLASH_REGION
    )
    foreach(option ${TFM_CMAKE_OPTIONS_PLATFORM_FLAG})
      if (CONFIG_${option})
        set(val "ON")
      else()
        set(val "OFF")
      endif()
      list(APPEND TFM_CMAKE_ARGS -D${option}=${val})
    endforeach()
  endif()

  if(CONFIG_TFM_BL2 AND CONFIG_TFM_MCUBOOT_PATH_LOCAL)
    # Supply path to MCUboot for TF-M build
    list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_PATH=${ZEPHYR_MCUBOOT_MODULE_DIR})
  endif()

  if(CONFIG_TFM_MCUBOOT_DATA_SHARING)
    list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_DATA_SHARING=ON)
  endif()

  if(CONFIG_FPU AND CONFIG_FP_HARDABI)
    list(APPEND TFM_CMAKE_ARGS -DCONFIG_TFM_ENABLE_FP=ON)
    # Note: This is not a cmake option in TF-M.
    # This should be specified by the platform in preload.cmake
    # This works as a workaround for the platforms that do not have this.
    list(APPEND TFM_CMAKE_ARGS -DCONFIG_TFM_FP_ARCH=${FPU_FOR_${GCC_M_CPU}})
  else()
    list(APPEND TFM_CMAKE_ARGS -DCONFIG_TFM_ENABLE_FP=OFF)
  endif()

  list(APPEND TFM_CMAKE_ARGS -DTFM_TESTS_REVISION_CHECKS=OFF)

  file(MAKE_DIRECTORY ${TFM_BINARY_DIR})
  add_custom_target(tfm_cmake
    DEPENDS ${TFM_BINARY_DIR}/CMakeCache.txt
  )
  add_custom_command(
    OUTPUT ${TFM_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND}
      -G${CMAKE_GENERATOR}
      -DTFM_TOOLCHAIN_FILE=${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/${TFM_TOOLCHAIN_FILE}
      -DCROSS_COMPILE=${TFM_TOOLCHAIN_PATH}/${TFM_TOOLCHAIN_PREFIX}
      -DCMAKE_BUILD_TYPE=${TFM_CMAKE_BUILD_TYPE}
      -DTFM_PLATFORM=${CONFIG_TFM_BOARD}
      -DCONFIG_TFM_BUILD_LOG_QUIET=ON
      -DCONFIG_TFM_MEMORY_USAGE_QUIET=OFF
      -DPython3_EXECUTABLE=${Python3_EXECUTABLE}
      ${TFM_CMAKE_ARGS}
      $<GENEX_EVAL:$<TARGET_PROPERTY:zephyr_property_target,TFM_CMAKE_OPTIONS>>
      -DMBEDCRYPTO_PATH=$<IF:$<BOOL:$<TARGET_PROPERTY:zephyr_property_target,TFM_MBEDCRYPTO_PATH>>,$<TARGET_PROPERTY:zephyr_property_target,TFM_MBEDCRYPTO_PATH>,${ZEPHYR_MBEDTLS_MODULE_DIR}>
      ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}
    WORKING_DIRECTORY ${TFM_BINARY_DIR}
    COMMAND_EXPAND_LISTS
  )

  include(ExternalProject)

  if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows)
    # Set number of parallel jobs for TF-M build to 1.
    # In some circumstances it has been experienced that building TF-M with
    # multiple parallel jobs then `permission denied` may occur. Root cause on
    # Windows has not been identified but current suspicion is around folder /
    # file lock mechanism. To ensure correct behaviour in all cases, limit
    # number of parallel jobs to 1.
    set(PARALLEL_JOBS -j 1)
  else()
	# Leave PARALLEL_JOBS unset and use the default number of
	# threads. Which is num_cores+2 on Ninja and MAKEFLAGS with Make.
  endif()

  ExternalProject_Add(
    tfm
    SOURCE_DIR ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}
    BINARY_DIR ${TFM_BINARY_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_COMMAND} --build . ${PARALLEL_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install .
    BUILD_ALWAYS True
    USES_TERMINAL_BUILD True
    DEPENDS tfm_cmake
    BUILD_BYPRODUCTS ${BUILD_BYPRODUCTS}
  )

  # Set TFM binary directory as target property on 'tfm'
  # This is the root of all TFM build artifacts.
  set_target_properties(tfm PROPERTIES TFM_BINARY_DIR ${TFM_BINARY_DIR})

  # Set TFM toolchain properties on 'tfm'
  set_target_properties(tfm PROPERTIES TFM_TOOLCHAIN_NS_FILE ${TFM_TOOLCHAIN_NS_FILE})
  set_target_properties(tfm PROPERTIES TFM_TOOLCHAIN_PREFIX  ${TFM_TOOLCHAIN_PREFIX})
  set_target_properties(tfm PROPERTIES TFM_TOOLCHAIN_PATH    ${TFM_TOOLCHAIN_PATH})

  # Set BL2 (MCUboot) executable file paths as target properties on 'tfm'
  # These files are produced by the TFM build system.
  if(CONFIG_TFM_BL2)
    set_target_properties(tfm PROPERTIES
      BL2_ELF_FILE ${BL2_ELF_FILE}
      BL2_BIN_FILE ${BL2_BIN_FILE}
      BL2_HEX_FILE ${BL2_HEX_FILE}
      )
  endif()

  # Set TFM S/NS executable file paths as target properties on 'tfm'
  # These files are produced by the TFM build system.
  # Note that the Nonsecure FW is replaced by the Zephyr app in regular Zephyr
  # builds.
  set_target_properties(tfm PROPERTIES
    TFM_S_ELF_FILE ${TFM_S_ELF_FILE}
    TFM_S_BIN_FILE ${TFM_S_BIN_FILE} # TFM Secure FW (unsigned)
    TFM_S_HEX_FILE ${TFM_S_HEX_FILE} # TFM Secure FW (unsigned)
    TFM_NS_BIN_FILE ${TFM_NS_BIN_FILE} # TFM Nonsecure FW (unsigned)
    TFM_NS_HEX_FILE ${TFM_NS_HEX_FILE} # TFM Nonsecure FW (unsigned)
    TFM_S_SIGNED_BIN_FILE ${TFM_S_SIGNED_BIN_FILE} # TFM Secure FW (signed)
    TFM_NS_SIGNED_BIN_FILE ${TFM_NS_SIGNED_BIN_FILE} # TFM Nonsecure FW (signed)
    TFM_S_NS_SIGNED_BIN_FILE ${TFM_S_NS_SIGNED_BIN_FILE} # Merged TFM Secure/Nonsecure FW (signed)
    )

  zephyr_library_named(tfm_api)

  zephyr_library_sources(
    src/zephyr_tfm_log.c
    interface/interface.c
    )

  # A dependency on tfm_s.hex for zephyr.elf will not cause a Zephyr re-link when
  # tfm_s.hex is updated, as the hex is not a direct input on the executable.
  # Instead we establish a source file dependency which ensures that tfm_api is
  # updated when there are changes in tfm itself, this again will trigger an re-link
  # of Zephyr.elf.
  set_property(SOURCE interface/interface.c APPEND PROPERTY OBJECT_DEPENDS ${TFM_S_HEX_FILE})

  # Non-Secure interface to request system reboot
  if (CONFIG_TFM_PARTITION_PLATFORM AND NOT CONFIG_TFM_PARTITION_PLATFORM_CUSTOM_REBOOT)
    zephyr_library_sources(src/reboot.c)
  endif()

  zephyr_library_sources_ifdef(CONFIG_TFM_PARTITION_PLATFORM                 ${TFM_INTERFACE_SOURCE_DIR}/tfm_platform_api.c)
  zephyr_library_sources_ifdef(CONFIG_TFM_PARTITION_PROTECTED_STORAGE        ${TFM_INTERFACE_SOURCE_DIR}/tfm_ps_api.c)
  zephyr_library_sources_ifdef(CONFIG_TFM_PARTITION_INTERNAL_TRUSTED_STORAGE ${TFM_INTERFACE_SOURCE_DIR}/tfm_its_api.c)
  zephyr_library_sources_ifdef(CONFIG_TFM_PARTITION_CRYPTO                   ${TFM_INTERFACE_SOURCE_DIR}/tfm_crypto_api.c)
  zephyr_library_sources_ifdef(CONFIG_TFM_PARTITION_INITIAL_ATTESTATION      ${TFM_INTERFACE_SOURCE_DIR}/tfm_attest_api.c)
  zephyr_library_sources_ifdef(CONFIG_TFM_PARTITION_FIRMWARE_UPDATE          ${TFM_INTERFACE_SOURCE_DIR}/tfm_fwu_api.c)

  zephyr_library_sources(${TFM_INTERFACE_SOURCE_DIR}/tfm_tz_psa_ns_api.c)

  if(CONFIG_SOC_FAMILY_NORDIC_NRF)
    zephyr_library_sources_ifdef(CONFIG_TFM_PARTITION_PLATFORM               ${TFM_INTERFACE_SOURCE_DIR}/tfm_ioctl_core_ns_api.c)
  endif()
  if(CONFIG_BOARD_RD_RW612_BGA)
    zephyr_library_sources_ifdef(CONFIG_TFM_PARTITION_PLATFORM               ${TFM_INTERFACE_SOURCE_DIR}/tfm_lds_api.c)
  endif()
  target_include_directories(tfm_api PUBLIC
    ${TFM_INTERFACE_INCLUDE_DIR}
    ${TFM_INTERFACE_INCLUDE_DIR}/crypto_keys
    ${ZEPHYR_BASE}/modules/mbedtls/configs
    )
  # Pass down the MbedTLS configuration file to use.
  target_compile_definitions(tfm_api PUBLIC
    MBEDTLS_CONFIG_FILE="${CONFIG_MBEDTLS_CFG_FILE}"
    )

  zephyr_library_link_libraries(
    ${TFM_INTERFACE_LIB_DIR}/s_veneers.o
    )

  # To ensure that generated include files are created before they are used.
  add_dependencies(zephyr_interface tfm)

  if (CONFIG_TFM_BL2)
    set(PREPROCESSED_FILE_S "${TFM_BINARY_DIR}/bl2/ext/mcuboot/CMakeFiles/signing_layout_s.dir/signing_layout_s.o")
    set(PREPROCESSED_FILE_S_NS "${TFM_BINARY_DIR}/bl2/ext/mcuboot/CMakeFiles/signing_layout_s.dir/signing_layout_s_ns.o")
    set(PREPROCESSED_FILE_NS "${TFM_BINARY_DIR}/bl2/ext/mcuboot/CMakeFiles/signing_layout_ns.dir/signing_layout_ns.o")
    set(TFM_MCUBOOT_DIR "${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/bl2/ext/mcuboot")
  endif()

  # Configure which format (full or hash) to include the public key in
  # the image manifest
  if(NOT DEFINED TFM_PUBLIC_KEY_FORMAT)
    set(TFM_PUBLIC_KEY_FORMAT "full")
  endif()

  if(DEFINED TFM_HEX_BASE_ADDRESS_S)
    set(HEX_ADDR_ARGS_S "--hex-addr=${TFM_HEX_BASE_ADDRESS_S}")
  endif()

  if(DEFINED TFM_HEX_BASE_ADDRESS_NS)
    set(HEX_ADDR_ARGS_NS "--hex-addr=${TFM_HEX_BASE_ADDRESS_NS}")
  endif()

  function(tfm_sign OUT_ARG SUFFIX PAD INPUT_FILE OUTPUT_FILE)
    if(PAD)
      set(pad_args --pad --pad-header)
    endif()
    # Secure + Non-secure images are signed the same way as a secure only
    # build, but with a different layout file.
    set(layout_file ${PREPROCESSED_FILE_${SUFFIX}})
    if(SUFFIX STREQUAL "S_NS")
      set(SUFFIX "S")
    endif()
    set (${OUT_ARG}
      # Add the MCUBoot script to the path so that if there is a version of imgtool in there then
      # it gets used over the system imgtool. Used so that imgtool from upstream
      # mcuboot is preferred over system imgtool
      ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts
      ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/wrapper/wrapper.py
      --layout ${layout_file}
      -k ${CONFIG_TFM_KEY_FILE_${SUFFIX}}
      --public-key-format ${TFM_PUBLIC_KEY_FORMAT}
      --align 1
      -v ${CONFIG_TFM_IMAGE_VERSION_${SUFFIX}}
      ${pad_args}
      ${HEX_ADDR_ARGS_${SUFFIX}}
      ${ADD_${SUFFIX}_IMAGE_MIN_VER}
      -s auto
      --measured-boot-record
      -H ${CONFIG_ROM_START_OFFSET}
      ${INPUT_FILE}
      ${OUTPUT_FILE}
      PARENT_SCOPE)
  endfunction()

  set(MERGED_FILE ${CMAKE_BINARY_DIR}/zephyr/tfm_merged.hex)
  set(S_NS_FILE ${CMAKE_BINARY_DIR}/zephyr/tfm_s_zephyr_ns.hex)
  set(S_NS_SIGNED_FILE ${CMAKE_BINARY_DIR}/zephyr/tfm_s_zephyr_ns_signed.hex)
  set(NS_SIGNED_FILE ${CMAKE_BINARY_DIR}/zephyr/zephyr_ns_signed.hex)
  set(S_SIGNED_FILE ${CMAKE_BINARY_DIR}/zephyr/tfm_s_signed.hex)

  if (CONFIG_TFM_USE_NS_APP)
    # Use the TF-M NS binary as the Non-Secure application firmware image
    set(NS_APP_FILE $<TARGET_PROPERTY:tfm,TFM_NS_HEX_FILE>)
  else()
    # Use the Zephyr binary as the Non-Secure application firmware image
    set(NS_APP_FILE ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME})
  endif()

  if (NOT CONFIG_TFM_BL2)
    # Merge tfm_s and zephyr (NS) image to a single binary.
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
        -o ${MERGED_FILE}
        $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>
        ${NS_APP_FILE}
    )

    set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
      ${MERGED_FILE}
    )

  elseif(CONFIG_TFM_MCUBOOT_IMAGE_NUMBER STREQUAL "1")
    tfm_sign(sign_cmd S_NS TRUE ${S_NS_FILE} ${S_NS_SIGNED_FILE})

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
        -o ${S_NS_FILE}
        $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>
        ${NS_APP_FILE}

      COMMAND ${sign_cmd}

      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
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
    if (CONFIG_TFM_USE_NS_APP)
      tfm_sign(sign_cmd_ns NS TRUE ${NS_APP_FILE} ${NS_SIGNED_FILE})
    else()
      tfm_sign(sign_cmd_ns NS FALSE ${NS_APP_FILE} ${NS_SIGNED_FILE})
    endif()

    tfm_sign(sign_cmd_s S TRUE $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE> ${S_SIGNED_FILE})

    #Create and sign for concatenated binary image, should align with the TF-M BL2
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${sign_cmd_ns}
      COMMAND ${sign_cmd_s}

      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
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

  if(CONFIG_TFM_DUMMY_PROVISIONING)
    message(WARNING
      "TFM_DUMMY_PROVISIONING is enabled:
      The device will be provisioned using dummy keys and is NOT secure!
      This is not suitable for production"
      )
  endif()

endif() # CONFIG_BUILD_WITH_TFM
