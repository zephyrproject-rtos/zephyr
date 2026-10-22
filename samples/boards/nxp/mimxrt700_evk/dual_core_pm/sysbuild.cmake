# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

if("${SB_CONFIG_SAMPLE_RT700_DUAL_CORE_PM_REMOTE_BOARD}" STREQUAL "")
  message(FATAL_ERROR "Target ${BOARD}/${BOARD_QUALIFIERS} is not supported by this sample. "
    "There is no remote board selected in Kconfig.sysbuild")
endif()

set(REMOTE_APP remote)

ExternalZephyrProject_Add(
  APPLICATION ${REMOTE_APP}
  SOURCE_DIR ${APP_DIR}/${REMOTE_APP}
  BOARD ${SB_CONFIG_SAMPLE_RT700_DUAL_CORE_PM_REMOTE_BOARD}
  BOARD_REVISION ${BOARD_REVISION}
)

set_config_int(${DEFAULT_IMAGE} CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SLEEP_SECONDS
  ${SB_CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SLEEP_SECONDS})
set_config_int(${REMOTE_APP} CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SLEEP_SECONDS
  ${SB_CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SLEEP_SECONDS})

# Only CPU0 sequences the cases, so only CPU0 needs the settle time.
set_config_int(${DEFAULT_IMAGE} CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SETTLE_SECONDS
  ${SB_CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SETTLE_SECONDS})

# The SoC copies the CPU1 image out of flash using the segment addresses in
# zephyr_image_info.h, which the CPU1 build generates. Point CPU0 at it through
# the SoC's own option; only this file knows what the remote image is called.
set_config_string(${DEFAULT_IMAGE} CONFIG_SECOND_CORE_MCUX_REMOTE_DIR
  ${APPLICATION_BINARY_DIR}/${REMOTE_APP}/zephyr/include/public)

add_dependencies(${DEFAULT_IMAGE} ${REMOTE_APP})
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} ${REMOTE_APP})
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} ${REMOTE_APP})
