# Copyright (c) 2023 Nordic Semiconductor ASA
# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

if("${SB_CONFIG_NET_CORE_BOARD}" STREQUAL "")
	message(FATAL_ERROR
	"Target ${BOARD} not supported for this sample. "
	"There is no remote board selected in Kconfig.sysbuild")
endif()

set(REMOTE_APP remote)

ExternalZephyrProject_Add(
	APPLICATION ${REMOTE_APP}
	SOURCE_DIR  ${APP_DIR}/${REMOTE_APP}
	BOARD       ${SB_CONFIG_NET_CORE_BOARD}
)

# Add dependencies so that the remote sample will be built first
# This is required because some primary cores need information from the
# remote core's build, such as the output image's LMA
add_dependencies(${DEFAULT_IMAGE} ${REMOTE_APP})
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} ${REMOTE_APP})
