# Board-specific CMake configuration for DSPIC33A curiosity board
# SPDX-License-Identifier: Apache-2.0
set(XCDSC_ZEPHYR_HOME ${XCDSC_TOOLCHAIN_PATH}/bin)
set(XCDSC_GNU_PREFIX xc-dsc-)

set(BOARD_FLASH_RUNNER ipecmd)
set(BOARD_DEBUG_RUNNER mdb)

if(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK128MC106)
    message(STATUS "device selected")
    board_runner_args(ipecmd "--device=33AK128MC106" "--flash-tool=PKOB4")
endif()

board_finalize_runner_args(ipecmd)

set_property(GLOBAL PROPERTY BOARD_SUPPORTS_DEBUGGER TRUE)
