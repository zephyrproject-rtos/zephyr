# Copyright (c) 2019, 2020 Linaro
# Copyright (c) 2020, 2021 Nordic Semiconductor ASA
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
  TFM_PARTITION_FIRMWARE_UPDATE
  )

# List of all crypto modules that can be enabled/disabled
# Corresponds to the *_MODULE_DISABLED configs in 'trusted-firmware-m/config/config_default.cmake'
set(TFM_CRYPTO_MODULES
  CRYPTO_RNG_MODULE
  CRYPTO_KEY_MODULE
  CRYPTO_AEAD_MODULE
  CRYPTO_MAC_MODULE
  CRYPTO_HASH_MODULE
  CRYPTO_CIPHER_MODULE
  CRYPTO_ASYM_ENCRYPT_MODULE
  CRYPTO_ASYM_SIGN_MODULE
  CRYPTO_KEY_DERIVATION_MODULE
  )


if (CONFIG_BUILD_WITH_TFM)
  if (CONFIG_TFM_IPC)
    list(APPEND TFM_CMAKE_ARGS -DTFM_LIB_MODEL=OFF)
    # PSA API awareness for the Non-Secure application
    target_compile_definitions(app PRIVATE "TFM_PSA_API")
  else()
    list(APPEND TFM_CMAKE_ARGS -DTFM_LIB_MODEL=ON)
  endif()
  if (CONFIG_TFM_REGRESSION_S)
    list(APPEND TFM_CMAKE_ARGS -DTEST_S=ON)
  endif()
  if (CONFIG_TFM_REGRESSION_NS)
    list(APPEND TFM_CMAKE_ARGS -DTEST_NS=ON)
  endif()
  if (CONFIG_TFM_BL2)
    list(APPEND TFM_CMAKE_ARGS -DBL2=TRUE)
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
  if (DEFINED TFM_PSA_TEST_SUITE)
    list(APPEND TFM_CMAKE_ARGS -DTEST_PSA_API=${TFM_PSA_TEST_SUITE})
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

  if (CONFIG_TFM_EXCEPTION_INFO_DUMP)
    list(APPEND TFM_CMAKE_ARGS -DTFM_EXCEPTION_INFO_DUMP=ON)
  else()
    list(APPEND TFM_CMAKE_ARGS -DTFM_EXCEPTION_INFO_DUMP=OFF)
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

  # Enable TFM crypto modules as specified in Kconfig
  foreach(module ${TFM_CRYPTO_MODULES})
    if (CONFIG_TFM_${module}_ENABLED)
      # list(APPEND TFM_ENABLED_CRYPTO_MODULES_ARG ${module})
      set(val "FALSE")
    else()
      set(val "TRUE")
    endif()
    list(APPEND TFM_CMAKE_ARGS -D${module}_DISABLED=${val})
  endforeach()

  set(TFM_BINARY_DIR ${CMAKE_BINARY_DIR}/tfm)

  set(VENEERS_FILE ${TFM_BINARY_DIR}/secure_fw/s_veneers.o)
  set(TFM_API_NS_PATH ${TFM_BINARY_DIR}/tf-m-tests/app/libtfm_api_ns.a)
  set(TFM_GENERATED_INCLUDES ${TFM_BINARY_DIR}/generated/interface/include)
  set(PLATFORM_NS_FILE ${TFM_BINARY_DIR}/platform/libplatform_ns.a)

  if (TFM_PSA_TEST_SUITE)
    set(PSA_TEST_VAL_FILE ${TFM_BINARY_DIR}/tf-m-tests/app/psa_api_tests/val/val_nspe.a)
    set(PSA_TEST_PAL_FILE ${TFM_BINARY_DIR}/tf-m-tests/app/psa_api_tests/platform/pal_nspe.a)
    set(COMBINE_DIR_STORAGE storage)
    set(COMBINE_DIR_PROTECTED_STORAGE storage)
    set(COMBINE_DIR_INTERNAL_TRUSTED_STORAGE storage)
    set(COMBINE_DIR_CRYPTO crypto)
    set(COMBINE_DIR_INITIAL_ATTESTATION initial_attestation)
    set(PSA_TEST_COMBINE_FILE ${TFM_BINARY_DIR}/tf-m-tests/app/psa_api_tests/dev_apis/${COMBINE_DIR_${TFM_PSA_TEST_SUITE}}/test_combine.a)
  endif()

  if(CONFIG_TFM_BL2)
    set(BL2_ELF_FILE ${TFM_BINARY_DIR}/bin/bl2.elf)
    set(BL2_BIN_FILE ${TFM_BINARY_DIR}/bin/bl2.bin)
    set(BL2_HEX_FILE ${TFM_BINARY_DIR}/bin/bl2.hex)
  endif()
  set(TFM_S_ELF_FILE ${TFM_BINARY_DIR}/bin/tfm_s.elf)
  set(TFM_S_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_s.bin)
  set(TFM_S_HEX_FILE ${TFM_BINARY_DIR}/bin/tfm_s.hex)
  set(TFM_NS_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_ns.bin)
  set(TFM_NS_HEX_FILE ${TFM_BINARY_DIR}/bin/tfm_ns.hex)
  set(TFM_S_SIGNED_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_s_signed.bin)
  set(TFM_NS_SIGNED_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_ns_signed.bin)
  set(TFM_S_NS_SIGNED_BIN_FILE ${TFM_BINARY_DIR}/bin/tfm_s_ns_signed.bin)

  set(BUILD_BYPRODUCTS
    ${VENEERS_FILE}
    ${TFM_API_NS_PATH}
    ${TFM_GENERATED_INCLUDES}/psa_manifest/sid.h
    ${PSA_TEST_VAL_FILE}
    ${PSA_TEST_PAL_FILE}
    ${PSA_TEST_COMBINE_FILE}
    ${PLATFORM_NS_FILE}
    ${BL2_ELF_FILE}
    ${BL2_BIN_FILE}
    ${BL2_HEX_FILE}
    ${TFM_S_ELF_FILE}
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

  if(CONFIG_BOARD_LPCXPRESSO55S69_CPU0)
    # Supply path to NXP HAL sources used for TF-M build
    set(TFM_PLATFORM_NXP_HAL_FILE_PATH ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/platform/ext/target/nxp/)
    list(APPEND TFM_CMAKE_ARGS -DTFM_PLATFORM_NXP_HAL_FILE_PATH=${TFM_PLATFORM_NXP_HAL_FILE_PATH})
  endif()

  if(CONFIG_TFM_BL2 AND CONFIG_TFM_MCUBOOT_PATH_LOCAL)
    # Supply path to MCUboot for TF-M build
    list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_PATH=${ZEPHYR_MCUBOOT_MODULE_DIR})
  endif()

  if(CONFIG_TFM_MCUBOOT_DATA_SHARING)
    list(APPEND TFM_CMAKE_ARGS -DMCUBOOT_DATA_SHARING=ON)
  endif()

  if(TFM_PSA_TEST_SUITE)
    list(APPEND TFM_CMAKE_ARGS
      -DPSA_TOOLCHAIN_FILE=${CMAKE_CURRENT_LIST_DIR}/psa/GNUARM.cmake
      -DTOOLCHAIN=INHERIT
      )
  endif()

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
      ${TFM_CMAKE_ARGS}
      $<GENEX_EVAL:$<TARGET_PROPERTY:zephyr_property_target,TFM_CMAKE_OPTIONS>>
      -DTFM_TEST_REPO_PATH=${ZEPHYR_TF_M_TESTS_MODULE_DIR}
      -DMBEDCRYPTO_PATH=$<IF:$<BOOL:$<TARGET_PROPERTY:zephyr_property_target,TFM_MBEDCRYPTO_PATH>>,$<TARGET_PROPERTY:zephyr_property_target,TFM_MBEDCRYPTO_PATH>,${ZEPHYR_MBEDTLS_MODULE_DIR}>
      -DPSA_ARCH_TESTS_PATH=${ZEPHYR_PSA_ARCH_TESTS_MODULE_DIR}
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
  # Non-Secure interface to request system reboot
  zephyr_library_sources_ifdef(CONFIG_TFM_PARTITION_PLATFORM src/reboot.c)
  zephyr_library_sources_ifndef(CONFIG_TFM_PSA_TEST_NONE src/zephyr_tfm_psa_test.c)

  zephyr_include_directories(
    ${TFM_GENERATED_INCLUDES}
    )

  target_include_directories(tfm_api PRIVATE
    ${TFM_BINARY_DIR}/install/interface/include
    )

  zephyr_library_link_libraries(
    ${PSA_TEST_VAL_FILE}
    ${PSA_TEST_PAL_FILE}
    ${PSA_TEST_COMBINE_FILE}
    ${PLATFORM_NS_FILE}
    ${TFM_API_NS_PATH}
    ${VENEERS_FILE}
    )

  # To ensure that generated include files are created before they are used.
  add_dependencies(zephyr_interface tfm)

  if (CONFIG_TFM_BL2)
    set(PREPROCESSED_FILE_S "${TFM_BINARY_DIR}/bl2/ext/mcuboot/CMakeFiles/signing_layout_s.dir/signing_layout_s.o")
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
    set (${OUT_ARG}
      # Add the MCUBoot script to the path so that if there is a version of imgtool in there then
      # it gets used over the system imgtool. Used so that imgtool from upstream
      # mcuboot is preferred over system imgtool
      ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts
      ${PYTHON_EXECUTABLE} ${TFM_MCUBOOT_DIR}/scripts/wrapper/wrapper.py
      --layout ${PREPROCESSED_FILE_${SUFFIX}}
      -k ${CONFIG_TFM_KEY_FILE_${SUFFIX}}
      --public-key-format ${TFM_PUBLIC_KEY_FORMAT}
      --align 1
      -v ${CONFIG_TFM_IMAGE_VERSION_${SUFFIX}}
      ${pad_args}
      ${HEX_ADDR_ARGS_${SUFFIX}}
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

  if (CONFIG_TFM_REGRESSION_NS)
    # Use the TF-M NS binary as the Non-Secure application firmware image
    set(NS_APP_FILE $<TARGET_PROPERTY:tfm,TFM_NS_HEX_FILE>)
  else()
    # Use the Zephyr binary as the Non-Secure application firmware image
    set(NS_APP_FILE ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME})
  endif()

  if (NOT CONFIG_TFM_BL2)
    # Merge tfm_s and zephyr (NS) image to a single binary.
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/mergehex.py
        -o ${MERGED_FILE}
        $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>
        ${NS_APP_FILE}
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
        ${NS_APP_FILE}

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
    if (CONFIG_TFM_REGRESSION_NS)
      tfm_sign(sign_cmd_ns NS TRUE ${NS_APP_FILE} ${NS_SIGNED_FILE})
    else()
      tfm_sign(sign_cmd_ns NS FALSE ${NS_APP_FILE} ${NS_SIGNED_FILE})
    endif()

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
