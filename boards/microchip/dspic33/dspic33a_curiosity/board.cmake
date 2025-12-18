# Copyright (c) 2025, Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0
set(XCDSC_ZEPHYR_HOME "${XCDSC_TOOLCHAIN_PATH}/bin")
set(XCDSC_GNU_PREFIX "xc-dsc-")

set(BOARD_FLASH_RUNNER "ipecmd")

if(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK128MC106)
  board_runner_args(ipecmd "--dev-id=33AK128MC106" "--flash-tool=PKOB4")
endif()

if(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK512MPS512)
  board_runner_args(ipecmd "--dev-id=33AK512MPS512" "--flash-tool=PKOB4")
endif()

board_finalize_runner_args(ipecmd)
