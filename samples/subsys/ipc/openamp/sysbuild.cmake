# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2022 NXP

# Add external project
ExternalZephyrProject_Add(
    APPLICATION openamp_remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_OPENAMP_REMOTE_BOARD}
  )

# Add dependencies so that the remote sample will be built first
# This is required because some primary cores need information from the
# remote core's build, such as the output image's LMA
add_dependencies(openamp openamp_remote)
sysbuild_add_dependencies(IMAGE CONFIGURE openamp openamp_remote)

# Make sure MCUboot is flashed first
sysbuild_add_dependencies(IMAGE FLASH openamp_remote mcuboot)
