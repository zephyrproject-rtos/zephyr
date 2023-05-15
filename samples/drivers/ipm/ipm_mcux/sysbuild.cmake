# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2022 NXP

# Add external project
ExternalZephyrProject_Add(
    APPLICATION ipm_mcux_remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_IPM_REMOTE_BOARD}
  )

# Add dependencies so that the remote sample will be built first
# This is required because some primary cores need information from the
# remote core's build, such as the output image's LMA
add_dependencies(ipm_mcux ipm_mcux_remote)
sysbuild_add_dependencies(IMAGE CONFIGURE ipm_mcux ipm_mcux_remote)

# Make sure MCUboot is flashed first
sysbuild_add_dependencies(IMAGE FLASH ipm_mcux_remote mcuboot)
