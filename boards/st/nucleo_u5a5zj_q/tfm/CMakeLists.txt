#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(NUCLEO_U5A5ZJ_Q_DIR ${CMAKE_CURRENT_LIST_DIR})
set(STM_COMMON_DIR ${PLATFORM_DIR}/ext/target/stm/common)

include(${STM_COMMON_DIR}/stm32u5xx/CMakeLists.txt)

#========================= Platform defs ===============================#

# Specify the location of platform specific build dependencies.
target_sources(tfm_s
    PRIVATE
        ${STM_COMMON_DIR}/stm32u5xx/Device/Source/startup_stm32u5xx_s.c
)

# cpuarch.cmake is used to set things that related to the platform that are both
install(FILES
    ${TARGET_PLATFORM_PATH}/cpuarch.cmake
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)

install(FILES
    ${STM_COMMON_DIR}/stm32u5xx/Device/Source/startup_stm32u5xx_ns.c
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/Device/Source
)

install(DIRECTORY
    ${TARGET_PLATFORM_PATH}/ns/
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)

install(DIRECTORY
    ${TARGET_PLATFORM_PATH}/include
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)

install(FILES
    ${TARGET_PLATFORM_PATH}/accelerator/crypto_accelerator_config.h
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/include
)

install(DIRECTORY
    ${STM_COMMON_DIR}/hal/accelerator/
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/include
    FILES_MATCHING PATTERN "*.h"
)

install(FILES
    ${NUCLEO_U5A5ZJ_Q_DIR}/partition/flash_layout.h
    ${NUCLEO_U5A5ZJ_Q_DIR}/partition/region_defs.h
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/partition
)

if(BL2)
    target_sources(bl2
        PRIVATE
            ${STM_COMMON_DIR}/stm32u5xx/Device/Source/startup_stm32u5xx_bl2.c
            ${STM_COMMON_DIR}/hal/provision/nvm_init.c
            ${STM_COMMON_DIR}/hal/provision/nvmcnt_init.c
            ${NUCLEO_U5A5ZJ_Q_DIR}/keys/otp_provision.c
    )
endif()
#install flash layout for postbuild.sh
install(FILES
    ${NUCLEO_U5A5ZJ_Q_DIR}/partition/flash_layout.h
    ${NUCLEO_U5A5ZJ_Q_DIR}/partition/region_defs.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
set (BL2_FILE_TO_PREPROCESS ${CMAKE_CURRENT_BINARY_DIR}/image_macros_to_preprocess_bl2.c)
file(WRITE ${BL2_FILE_TO_PREPROCESS} ${BL2_PREPROCESSING})

install(FILES
    ${BL2_FILE_TO_PREPROCESS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
