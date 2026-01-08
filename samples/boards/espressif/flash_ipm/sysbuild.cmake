# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2024 Espressif

# Prepare the full board name to be used for the remote target
string(REPLACE "procpu" "appcpu" REMOTE_CPU "${BOARD_QUALIFIERS}")
string(CONFIGURE "${BOARD}${REMOTE_CPU}" IPM_REMOTE_BOARD)

if(${REMOTE_CPU} STREQUAL ${BOARD_QUALIFIERS})
  # Make sure the remote build is using different target than host CPU
  message(FATAL_ERROR "BOARD_QUALIFIERS name error. Please check the target board name string.")
endif()

# Add external project
ExternalZephyrProject_Add(
    APPLICATION flash_ipm_remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${IPM_REMOTE_BOARD}
  )

# Add dependencies so that the remote sample will be built first
# This is required because some primary cores need information from the
# remote core's build, such as the output image's LMA
add_dependencies(flash_ipm flash_ipm_remote)
sysbuild_add_dependencies(CONFIGURE flash_ipm flash_ipm_remote)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  # Make sure MCUboot is flashed first
  sysbuild_add_dependencies(FLASH flash_ipm_remote mcuboot)
endif()
