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
