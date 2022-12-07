# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2022 NXP

# Add external project
ExternalZephyrProject_Add(
    APPLICATION ipm_mcux_remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_IPM_REMOTE_BOARD}
  )

# Add a dependency so that the remote sample will be built and flashed first
# This is required because some primary cores need information from the
# remote core's build, such as the output image's LMA
add_dependencies(ipm_mcux ipm_mcux_remote)
# Place remote image first in the image list
set(IMAGES "ipm_mcux_remote" ${IMAGES})
