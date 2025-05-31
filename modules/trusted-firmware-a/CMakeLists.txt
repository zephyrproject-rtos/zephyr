# Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_BUILD_WITH_TFA)
  set(TFA_BINARY_DIR ${CMAKE_BINARY_DIR}/tfa)

  file(MAKE_DIRECTORY ${TFA_BINARY_DIR})

  include(ExternalProject)

  set(tfa_image_info MAP "name: tfa, source-dir: ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}")
  build_info(images VALUE ${tfa_image_info})

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

  if (CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(TFA_BUILD_DEBUG "1")
  else()
    set(TFA_BUILD_DEBUG "0")
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND make -C ${ZEPHYR_TRUSTED_FIRMWARE_A_MODULE_DIR}
            DEBUG=${TFA_BUILD_DEBUG}
            CROSS_COMPILE=${CROSS_COMPILE}
            BUILD_BASE=${TFA_BINARY_DIR} PLAT=${TFA_PLAT}
            BL33=${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}
            all fip
  )
endif()
