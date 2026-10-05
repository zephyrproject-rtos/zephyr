# Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFA)
  set(TFA_BINARY_DIR ${CMAKE_BINARY_DIR}/tfa)

  file(MAKE_DIRECTORY ${TFA_BINARY_DIR})

  include(ExternalProject)

  set(tfa_image_info MAP "name: tfa, source-dir: ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}")
  build_info(images VALUE ${tfa_image_info})

  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(TFA_BUILD_DEBUG "1")
  else()
    set(TFA_BUILD_DEBUG "0")
  endif()

  if(CONFIG_SOC_SERIES_AMEBASMART)
    # The AmebaSmart ATF has its own build flow (project_ap tree, PROJECT_DIR,
    # bl1/image targets) instead of the generic TF-A one below.
    if(NOT DEFINED ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR)
      message(FATAL_ERROR
        "ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR not set. "
        "Ensure tf-a is listed in the west manifest and run 'west update'."
      )
    endif()

    # External inputs the ATF Makefile expects.
    set(TFA_PROJECT_DIR ${TFA_BINARY_DIR}/project_ap)
    file(MAKE_DIRECTORY ${TFA_PROJECT_DIR}/lib/soc)
    file(MAKE_DIRECTORY ${TFA_PROJECT_DIR}/image)

    # ATF includes .config_ca32 but needs none of its values here.
    file(WRITE ${TFA_PROJECT_DIR}/.config_ca32
      "# Stub .config_ca32 for the Zephyr ATF build\n"
    )

    # ATF's platform_def.h includes platform_autoconf.h from
    # ${BUILD_BASE}/../menuconfig/project_ap.
    set(TFA_AUTOCONFIG_DIR ${CMAKE_BINARY_DIR}/menuconfig/project_ap)
    file(MAKE_DIRECTORY ${TFA_AUTOCONFIG_DIR})
    file(WRITE ${TFA_AUTOCONFIG_DIR}/platform_autoconf.h
      "/* Stub platform_autoconf.h for the Zephyr ATF build */\n"
    )

    # platform.mk expects these paths relative to the ATF tree:
    #   ../../common/include          -> hal_realtek ameba common include
    #   ../../../os/os_wrapper/include -> hal_realtek os_wrapper include
    set(RTK_COMMON_INCLUDE ${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/common/include)
    set(RTK_OS_WRAPPER_INCLUDE ${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/common/os_wrapper/include)
    set(TFA_COMMON_LINK ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}/../../common/include)
    set(TFA_OS_WRAPPER_LINK ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}/../../../os/os_wrapper/include)
    file(MAKE_DIRECTORY ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}/../../common)
    file(MAKE_DIRECTORY ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}/../../../os/os_wrapper)
    if(NOT EXISTS ${TFA_COMMON_LINK})
      execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${RTK_COMMON_INCLUDE} ${TFA_COMMON_LINK})
    endif()
    if(NOT EXISTS ${TFA_OS_WRAPPER_LINK})
      execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${RTK_OS_WRAPPER_INCLUDE} ${TFA_OS_WRAPPER_LINK})
    endif()

    # sp_min links against the prebuilt libpm.a blob.
    set(LIBPM_BLOB ${ZEPHYR_HAL_REALTEK_MODULE_DIR}/zephyr/blobs/ameba/amebasmart/lib/libpm.a)
    if(EXISTS ${LIBPM_BLOB})
      file(COPY ${LIBPM_BLOB} DESTINATION ${TFA_PROJECT_DIR}/lib/soc/)
    else()
      message(WARNING "libpm.a not found at ${LIBPM_BLOB}, ATF build may fail")
    endif()

    # BL33 is the final Zephyr image, so ATF is built as a post-build command.
    ExternalProject_Add(
      tfa
      SOURCE_DIR ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}
      BINARY_DIR ${TFA_BINARY_DIR}
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      BUILD_ALWAYS True
      USES_TERMINAL_BUILD True
    )

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${CMAKE_COMMAND} -E env
        make -C ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}
        DEBUG=${TFA_BUILD_DEBUG}
        CROSS_COMPILE=${CROSS_COMPILE}
        PROJECT_DIR=${TFA_PROJECT_DIR}
        BUILD_BASE=${TFA_BINARY_DIR}
        BL33=${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}
        SOC_DIR=${ZEPHYR_HAL_REALTEK_MODULE_DIR}/ameba/amebasmart/source/fwlib/include
        bl1 image
    )
  else()
    # ARMv9-A is AArch64-only, disable AArch32 register context
    if(CONFIG_ARMV9_A)
      set(TFA_AARCH32_REGS "CTX_INCLUDE_AARCH32_REGS=0")
    else()
      set(TFA_AARCH32_REGS "")
    endif()

    # Set default TF-A build parameters that can be overridden by boards
    if(NOT DEFINED TFA_EXTRA_ARGS)
      set(TFA_EXTRA_ARGS "")
    endif()

    if(NOT DEFINED TFA_PLAT)
      set(TFA_PLAT "${CONFIG_TFA_PLATFORM}")
    endif()

    find_program(MAKE_EXECUTABLE NAMES make REQUIRED)

    if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
      set(MAKE_COMMAND "$(MAKE)")
    else()
      set(MAKE_COMMAND ${MAKE_EXECUTABLE})
    endif()

    # Build base command
    set(TFA_BUILD_CMD_COMMON
      ${MAKE_COMMAND} -C ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}
        DEBUG=${TFA_BUILD_DEBUG}
        CROSS_COMPILE=${CROSS_COMPILE}
        BUILD_BASE=${TFA_BINARY_DIR}
        PLAT=${TFA_PLAT}
        ${TFA_AARCH32_REGS}
        ${TFA_EXTRA_ARGS}
    )

    set(TFA_BUILD_CMD "")
    set(TFA_POST_BUILD_MAKE_ARGS "")

    if(CONFIG_TFA_BUILD_FIP)
      set(TFA_BUILD_CMD ${CMAKE_COMMAND} -E true)
      if(CONFIG_TFA_FIP_USE_BL33_ZEPHYR)
        list(APPEND TFA_POST_BUILD_MAKE_ARGS
          BL33=${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}
        )
      endif()

      list(APPEND TFA_POST_BUILD_MAKE_ARGS all fip)
    else()
      set(TFA_BUILD_CMD ${TFA_BUILD_CMD_COMMON} all)
    endif()

    ExternalProject_Add(
      tfa
      SOURCE_DIR ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}
      BINARY_DIR ${TFA_BINARY_DIR}
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ${TFA_BUILD_CMD}
      INSTALL_COMMAND ""
      BUILD_ALWAYS True
      USES_TERMINAL_BUILD True
    )

    if(NOT "${TFA_POST_BUILD_MAKE_ARGS}" STREQUAL "")
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
        COMMAND ${TFA_BUILD_CMD_COMMON}
          ${TFA_POST_BUILD_MAKE_ARGS}
      )
    endif()
  endif()
endif()
