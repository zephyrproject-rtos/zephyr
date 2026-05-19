# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2026 Espressif Systems (Shanghai) Co., Ltd.

# Add external project
ExternalZephyrProject_Add(
    APPLICATION timer_wakeup_lpcore
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_ULP_REMOTE_BOARD}
  )

# Add dependencies so that the remote sample will be built first
# This is required because some primary cores need information from the
# remote core's build, such as the output image's LMA
add_dependencies(timer_wakeup timer_wakeup_lpcore)
sysbuild_add_dependencies(CONFIGURE timer_wakeup timer_wakeup_lpcore)
sysbuild_add_dependencies(FLASH timer_wakeup_lpcore timer_wakeup)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  # Make sure MCUboot is flashed first
  sysbuild_add_dependencies(FLASH timer_wakeup mcuboot)
endif()
